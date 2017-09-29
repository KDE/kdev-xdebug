/*
* XDebug Debugger Support
*
* Copyright 2009 Niko Sams <niko.sams@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "launchconfigurationpage.h"

#include <QVBoxLayout>

#include <QIcon>
#include <KLocalizedString>

#include "ui_launchconfigurationpage.h"
#include "launchconfig.h"
#include "debugsession.h"

namespace XDebug
{

KDevelop::LaunchConfigurationPage* ConfigPageFactory::createWidget( QWidget* parent )
{
    return new ConfigPage( parent );
}

ConfigPage::ConfigPage( QWidget* parent )
    : LaunchConfigurationPage(parent)
{
    m_ui = new Ui::LaunchConfigurationWidget;
    m_ui->setupUi(this);

    connect(m_ui->pathMappings, SIGNAL(changed()), SIGNAL(changed()));
}

QIcon ConfigPage::icon() const
{
    return QIcon();
}

void ConfigPage::loadFromConfiguration( const KConfigGroup& cfg, KDevelop::IProject*  )
{
    m_ui->pathMappings->loadFromConfiguration(cfg);
    m_ui->remoteHost->setText(cfg.readEntry("RemoteHost", QString()));
    m_ui->remotePort->setValue(cfg.readEntry("RemotePort", 9000));
}

void ConfigPage::saveToConfiguration( KConfigGroup cfg, KDevelop::IProject* ) const
{
    m_ui->pathMappings->saveToConfiguration(cfg);
    cfg.writeEntry("RemoteHost", m_ui->remoteHost->text());
    cfg.writeEntry("RemotePort", m_ui->remotePort->value());
}

QString ConfigPage::title() const
{
    return i18n( "XDebug Configuration" );
}

}

#include "launchconfigurationpage.moc"
