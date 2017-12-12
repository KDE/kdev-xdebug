/*
 * XDebug Debugger Support
 *
 * Copyright 1999-2001 John Birch <jbb@kdevelop.org>
 * Copyright 2001 by Bernd Gehrmann <bernd@kdevelop.org>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
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

#ifndef XDEBUGPLUGIN_H
#define XDEBUGPLUGIN_H

#include <QPointer>
#include <QByteArray>
#include <QLabel>
#include <QVariant>

#include <KConfigGroup>
#include <KTextEditor/Cursor>

#include <interfaces/iplugin.h>
#include <debugger/interfaces/idebugsession.h>

class KProcess;

namespace XDebug {
class DebugSession;
class Server;

class XDebugPlugin
    : public KDevelop::IPlugin
{
    Q_OBJECT

public:
    XDebugPlugin(QObject* parent, const QVariantList& = QVariantList());
    ~XDebugPlugin() override;
    DebugSession* createSession() const;

private:
    Server* m_server;
};
}

#endif
