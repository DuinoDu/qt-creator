/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "uncrustifyoptionspage.h"
#include "ui_uncrustifyoptionspage.h"

#include "uncrustifyconstants.h"
#include "uncrustifysettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

#include <coreplugin/icore.h>

#include <QTextStream>

namespace Beautifier {
namespace Internal {
namespace Uncrustify {

UncrustifyOptionsPageWidget::UncrustifyOptionsPageWidget(UncrustifySettings *settings,
                                                         QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UncrustifyOptionsPage),
    m_settings(settings)
{
    ui->setupUi(this);
    ui->useHomeFile->setText(ui->useHomeFile->text().replace(
                                 "HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));
    ui->command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui->command->setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                          Constants::Uncrustify::DISPLAY_NAME));
    connect(ui->command, &Utils::PathChooser::validChanged, ui->options, &QWidget::setEnabled);
    ui->configurations->setSettings(m_settings);
}

UncrustifyOptionsPageWidget::~UncrustifyOptionsPageWidget()
{
    delete ui;
}

void UncrustifyOptionsPageWidget::restore()
{
    ui->command->setPath(m_settings->command());
    ui->useOtherFiles->setChecked(m_settings->useOtherFiles());
    ui->useHomeFile->setChecked(m_settings->useHomeFile());
    ui->useCustomStyle->setChecked(m_settings->useCustomStyle());
    ui->configurations->setCurrentConfiguration(m_settings->customStyle());
    ui->formatEntireFileFallback->setChecked(m_settings->formatEntireFileFallback());
}

void UncrustifyOptionsPageWidget::apply()
{
    m_settings->setCommand(ui->command->path());
    m_settings->setUseOtherFiles(ui->useOtherFiles->isChecked());
    m_settings->setUseHomeFile(ui->useHomeFile->isChecked());
    m_settings->setUseCustomStyle(ui->useCustomStyle->isChecked());
    m_settings->setCustomStyle(ui->configurations->currentConfiguration());
    m_settings->setFormatEntireFileFallback(ui->formatEntireFileFallback->isChecked());
    m_settings->save();
}

UncrustifyOptionsPage::UncrustifyOptionsPage(UncrustifySettings *settings, QObject *parent) :
    IOptionsPage(parent),
    m_settings(settings)
{
    setId(Constants::Uncrustify::OPTION_ID);
    setDisplayName(tr("Uncrustify"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", Constants::OPTION_TR_CATEGORY));
    setCategoryIcon(Constants::OPTION_CATEGORY_ICON);
}

QWidget *UncrustifyOptionsPage::widget()
{
    m_settings->read();

    if (!m_widget)
        m_widget = new UncrustifyOptionsPageWidget(m_settings);
    m_widget->restore();

    return m_widget;
}

void UncrustifyOptionsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void UncrustifyOptionsPage::finish()
{
}

} // namespace Uncrustify
} // namespace Internal
} // namespace Beautifier
