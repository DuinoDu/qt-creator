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

#pragma once

#include "threaddata.h"

#include <utils/treemodel.h>

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class GdbMi;
class ThreadItem;

class ThreadsHandler : public Utils::TreeModel
{
    Q_OBJECT

public:
    ThreadsHandler();

    int currentThreadIndex() const;
    ThreadId currentThread() const;
    ThreadId threadAt(int index) const;
    void setCurrentThread(ThreadId id);
    QByteArray pidForGroupId(const QByteArray &groupId) const;

    void updateThread(const ThreadData &threadData);
    void updateThreads(const GdbMi &data);

    void removeThread(ThreadId threadId);
    void setThreads(const Threads &threads);
    void removeAll();
    ThreadData thread(ThreadId id) const;
    QAbstractItemModel *model();

    void notifyGroupCreated(const QByteArray &groupId, const QByteArray &pid);
    bool notifyGroupExited(const QByteArray &groupId); // Returns true when empty.

    // Clear out all frame information
    void notifyRunning(const QByteArray &data);
    void notifyRunning(ThreadId threadId);
    void notifyAllRunning();

    void notifyStopped(const QByteArray &data);
    void notifyStopped(ThreadId threadId);
    void notifyAllStopped();

    void resetLocation();
    void scheduleResetLocation();

    void foreachThread(const std::function<void(ThreadItem *item)> &func);

private:
    void updateThreadBox();

    void sort(int column, Qt::SortOrder order);

    ThreadId m_currentId;
    bool m_resetLocationScheduled;
    QHash<QByteArray, QByteArray> m_pidForGroupId;
};

} // namespace Internal
} // namespace Debugger
