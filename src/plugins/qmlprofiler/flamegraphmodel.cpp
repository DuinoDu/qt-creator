/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "flamegraphmodel.h"

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVector>
#include <QString>
#include <QStack>
#include <QQueue>
#include <QSet>

namespace QmlProfiler {
namespace Internal {

FlameGraphModel::FlameGraphModel(QmlProfilerModelManager *modelManager,
                                 QObject *parent) : QAbstractItemModel(parent)
{
    m_modelManager = modelManager;
    connect(modelManager->qmlModel(), &QmlProfilerDataModel::changed,
            this, [this](){loadData();});
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, [this](int typeId, int, int){loadNotes(typeId, true);});
    m_modelId = modelManager->registerModelProxy();

    // We're iterating twice in loadData.
    modelManager->setProxyCountWeight(m_modelId, 2);

    m_acceptedTypes << Compiling << Creating << Binding << HandlingSignal << Javascript;
    modelManager->announceFeatures(m_modelId, Constants::QML_JS_RANGE_FEATURES);
}

void FlameGraphModel::clear()
{
    m_modelManager->modelProxyCountUpdated(m_modelId, 0, 1);
    m_stackBottom = FlameGraphData();
    m_typeIdsWithNotes.clear();
}

void FlameGraphModel::loadNotes(int typeIndex, bool emitSignal)
{
    QSet<int> changedNotes;
    Timeline::TimelineNotesModel *notes = m_modelManager->notesModel();
    if (typeIndex == -1) {
        changedNotes = m_typeIdsWithNotes;
        m_typeIdsWithNotes.clear();
        for (int i = 0; i < notes->count(); ++i)
            m_typeIdsWithNotes.insert(notes->typeId(i));
        changedNotes += m_typeIdsWithNotes;
    } else {
        changedNotes.insert(typeIndex);
        if (notes->byTypeId(typeIndex).isEmpty())
            m_typeIdsWithNotes.remove(typeIndex);
        else
            m_typeIdsWithNotes.insert(typeIndex);
    }

    if (emitSignal)
        emit dataChanged(QModelIndex(), QModelIndex(), QVector<int>() << NoteRole);
}

void FlameGraphModel::loadData(qint64 rangeStart, qint64 rangeEnd)
{
    const bool checkRanges = (rangeStart != -1) && (rangeEnd != -1);
    if (m_modelManager->state() == QmlProfilerModelManager::ClearingData) {
        beginResetModel();
        clear();
        endResetModel();
        return;
    } else if (m_modelManager->state() != QmlProfilerModelManager::ProcessingData &&
               m_modelManager->state() != QmlProfilerModelManager::Done) {
        return;
    }

    beginResetModel();
    clear();

    const QVector<QmlEvent> &eventList = m_modelManager->qmlModel()->events();
    const QVector<QmlEventType> &typesList = m_modelManager->qmlModel()->eventTypes();

    // used by binding loop detection
    QStack<const QmlEvent *> callStack;
    callStack.append(0);
    FlameGraphData *stackTop = &m_stackBottom;

    for (int i = 0; i < eventList.size(); ++i) {
        const QmlEvent *event = &eventList[i];
        int typeIndex = event->typeIndex();
        const QmlEventType *type = &typesList[typeIndex];

        if (!m_acceptedTypes.contains(type->rangeType))
            continue;

        if (checkRanges) {
            if ((event->timestamp() + event->duration() < rangeStart)
                    || (event->timestamp() > rangeEnd))
                continue;
        }

        const QmlEvent *potentialParent = callStack.top();
        while (potentialParent &&
               potentialParent->timestamp() + potentialParent->duration() <= event->timestamp()) {
            callStack.pop();
            stackTop = stackTop->parent;
            potentialParent = callStack.top();
        }

        callStack.push(event);
        stackTop = pushChild(stackTop, event);

        m_modelManager->modelProxyCountUpdated(m_modelId, i, eventList.count());
    }

    foreach (FlameGraphData *child, m_stackBottom.children)
        m_stackBottom.duration += child->duration;

    loadNotes(-1, false);

    m_modelManager->modelProxyCountUpdated(m_modelId, 1, 1);
    endResetModel();
}

static QString nameForType(RangeType typeNumber)
{
    switch (typeNumber) {
    case Painting: return FlameGraphModel::tr("Paint");
    case Compiling: return FlameGraphModel::tr("Compile");
    case Creating: return FlameGraphModel::tr("Create");
    case Binding: return FlameGraphModel::tr("Binding");
    case HandlingSignal: return FlameGraphModel::tr("Signal");
    case Javascript: return FlameGraphModel::tr("JavaScript");
    default: return QString();
    }
}

QVariant FlameGraphModel::lookup(const FlameGraphData &stats, int role) const
{
    switch (role) {
    case TypeIdRole: return stats.typeIndex;
    case NoteRole: {
        QString ret;
        if (!m_typeIdsWithNotes.contains(stats.typeIndex))
            return ret;
        QmlProfilerNotesModel *notes = m_modelManager->notesModel();
        foreach (const QVariant &item, notes->byTypeId(stats.typeIndex)) {
            if (ret.isEmpty())
                ret = notes->text(item.toInt());
            else
                ret += QChar::LineFeed + notes->text(item.toInt());
        }
        return ret;
    }
    case DurationRole: return stats.duration;
    case CallCountRole: return stats.calls;
    case TimePerCallRole: return stats.duration / stats.calls;
    case TimeInPercentRole: return stats.duration * 100 / m_stackBottom.duration;
    default: break;
    }

    if (stats.typeIndex != -1) {
        const QVector<QmlEventType> &typeList = m_modelManager->qmlModel()->eventTypes();
        const QmlEventType &type = typeList[stats.typeIndex];

        switch (role) {
        case FilenameRole: return type.location.filename;
        case LineRole: return type.location.line;
        case ColumnRole: return type.location.column;
        case TypeRole: return nameForType(type.rangeType);
        case RangeTypeRole: return type.rangeType;
        case DetailsRole: return type.data.isEmpty() ?
                        FlameGraphModel::tr("Source code not available") : type.data;
        case LocationRole: return type.displayName;
        default: return QVariant();
        }
    } else {
        return QVariant();
    }
}

FlameGraphData::FlameGraphData(FlameGraphData *parent, int typeIndex, qint64 duration) :
    duration(duration), calls(1), typeIndex(typeIndex), parent(parent) {}

FlameGraphData::~FlameGraphData()
{
    qDeleteAll(children);
}

FlameGraphData *FlameGraphModel::pushChild(
        FlameGraphData *parent, const QmlEvent *data)
{
    foreach (FlameGraphData *child, parent->children) {
        if (child->typeIndex == data->typeIndex()) {
            ++child->calls;
            child->duration += data->duration();
            return child;
        }
    }

    FlameGraphData *child = new FlameGraphData(parent, data->typeIndex(), data->duration());
    parent->children.append(child);
    return child;
}

QModelIndex FlameGraphModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        FlameGraphData *parentData = static_cast<FlameGraphData *>(parent.internalPointer());
        return createIndex(row, column, parentData->children[row]);
    } else {
        return createIndex(row, column, row >= 0 ? m_stackBottom.children[row] : 0);
    }
}

QModelIndex FlameGraphModel::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        FlameGraphData *childData = static_cast<FlameGraphData *>(child.internalPointer());
        return createIndex(0, 0, childData->parent);
    } else {
        return QModelIndex();
    }
}

int FlameGraphModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        FlameGraphData *parentData = static_cast<FlameGraphData *>(parent.internalPointer());
        return parentData->children.count();
    } else {
        return m_stackBottom.children.count();
    }
}

int FlameGraphModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant FlameGraphModel::data(const QModelIndex &index, int role) const
{
    FlameGraphData *data = static_cast<FlameGraphData *>(index.internalPointer());
    return lookup(data ? *data : m_stackBottom, role);
}

QHash<int, QByteArray> FlameGraphModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names[TypeIdRole] = "typeId";
    names[TypeRole] = "type";
    names[DurationRole] = "duration";
    names[CallCountRole] = "callCount";
    names[DetailsRole] = "details";
    names[FilenameRole] = "filename";
    names[LineRole] = "line";
    names[ColumnRole] = "column";
    names[NoteRole] = "note";
    names[TimePerCallRole] = "timePerCall";
    names[TimeInPercentRole] = "timeInPercent";
    names[RangeTypeRole] = "rangeType";
    return names;
}

} // namespace Internal
} // namespace QmlProfiler
