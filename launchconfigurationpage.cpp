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

#include <KIcon>
#include <KLocalizedString>

#include <debugger/util/pathmappings.h>

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
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    this->setLayout(verticalLayout);

    m_pathMappings = new KDevelop::PathMappingsWidget;
    connect(m_pathMappings, SIGNAL(changed()), SIGNAL(changed()));
    verticalLayout->addWidget(m_pathMappings);
}

KIcon ConfigPage::icon() const
{
    return KIcon();
}

void ConfigPage::loadFromConfiguration( const KConfigGroup& cfg, KDevelop::IProject*  )
{
    m_pathMappings->loadFromConfiguration(cfg);
}

void ConfigPage::saveToConfiguration( KConfigGroup cfg, KDevelop::IProject* ) const
{
    m_pathMappings->saveToConfiguration(cfg);
}

QString ConfigPage::title() const
{
    return i18n( "XDebug Configuration" );
}

}

#include "launchconfigurationpage.moc"
