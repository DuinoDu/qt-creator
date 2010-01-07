/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include <QtGui/QWidget>
#include <QtGui/QFontDatabase>

#include <coreplugin/dialogs/ioptionspage.h>

#include "ui_generalsettingspage.h"

class BookmarkManager;

QT_FORWARD_DECLARE_CLASS(QFont)
QT_FORWARD_DECLARE_CLASS(QHelpEngine)

namespace Help {
namespace Internal {

class CentralWidget;

class GeneralSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettingsPage(QHelpEngine *helpEngine, CentralWidget *centralWidget,
        BookmarkManager *bookmarkManager);

    QString id() const;
    virtual QString displayName() const;
    QString category() const;
    QString displayCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    virtual bool matches(const QString &s) const;

signals:
    void fontChanged();

private slots:
    void setCurrentPage();
    void setBlankPage();
    void setDefaultPage();
    void importBookmarks();
    void exportBookmarks();

private:
    void updateFontSize();
    void updateFontStyle();
    void updateFontFamily();
    int closestPointSizeIndex(int desiredPointSize) const;

private:
    QWidget *m_currentPage;
    QHelpEngine *m_helpEngine;
    CentralWidget *m_centralWidget;
    BookmarkManager *m_bookmarkManager;

    QFont font;
    QFontDatabase fontDatabase;

    Ui::GeneralSettingsPage m_ui;
    QString m_searchKeywords;
};

    }   // Internal
}   // Help

#endif  // GENERALSETTINGSPAGE_H
