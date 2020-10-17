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

#include "debugsession.h"

#include <QTime>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>

#include <QDebug>
#include <KLocalizedString>

#include <interfaces/ilaunchconfiguration.h>
#include <debugger/util/pathmappings.h>

#include "connection.h"
#include "breakpointcontroller.h"
#include "framestackmodel.h"
#include "variablecontroller.h"
#include "launchconfig.h"
#include "debuggerdebug.h"

namespace XDebug {
DebugSession::DebugSession()
    : KDevelop::IDebugSession()
    , m_breakpointController(new BreakpointController(this))
    , m_variableController(new VariableController(this))
    , m_frameStackModel(new FrameStackModel(this))
    , m_server(nullptr)
    , m_connection(nullptr)
    , m_launchConfiguration(nullptr)
    , m_acceptMultipleConnections(false)
{
}

DebugSession::~DebugSession()
{
    emit finished();
}

void DebugSession::setLaunchConfiguration(KDevelop::ILaunchConfiguration* cfg)
{
    m_launchConfiguration = cfg;
}

void DebugSession::setAcceptMultipleConnections(bool v)
{
    m_acceptMultipleConnections = v;
}

bool DebugSession::listenForConnection(QString& error)
{
    Q_ASSERT(!m_server);
    m_server = new QTcpServer(this);
    qCDebug(KDEV_PHP_DEBUGGER);
    int remotePortSetting = m_launchConfiguration->config().readEntry("RemotePort", 9000);
    if (m_server->listen(QHostAddress::Any, remotePortSetting)) {
        connect(m_server, &QTcpServer::newConnection, this, &DebugSession::incomingConnection);
        // avoid 'debug launch' button
        stateChanged(ActiveState);
    } else {
        error = i18n("Opening port %1 failed: %2.", remotePortSetting, m_server->errorString());
        qCWarning(KDEV_PHP_DEBUGGER) << "Error" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }
    return m_server->isListening();
}

void DebugSession::closeServer()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

void DebugSession::incomingConnection()
{
    qCDebug(KDEV_PHP_DEBUGGER);
    QTcpSocket* client = m_server->nextPendingConnection();

    if (m_connection) {
        m_connection->disconnect();
        m_connection->deleteLater();
        m_connection = nullptr;
    }

    m_connection = new Connection(client, this);
    connect(m_connection, &Connection::output, this, &DebugSession::output);
    connect(m_connection, &Connection::outputLine, this, &DebugSession::outputLine);
    connect(m_connection, &Connection::stateChanged, this, &DebugSession::stateChanged);
    connect(m_connection, &Connection::stateChanged, this, &DebugSession::_stateChanged);
    connect(m_connection, &Connection::currentPositionChanged,this, &DebugSession::currentPositionChanged);
    connect(m_connection, &Connection::closed, this,  &DebugSession::connectionClosed);

    if (!m_acceptMultipleConnections) {
        closeServer();
    }
}

void DebugSession::connectionClosed()
{
    Q_ASSERT(sender() == m_connection);

    if (m_acceptMultipleConnections && m_server && m_server->isListening() 
       ) {
        // clear variable widget
        emit stateChanged(NotStartedState);
        // avoid 'debug launch' button
        emit stateChanged(ActiveState);
    } else {
        m_connection->setState(DebugSession::EndedState);
    }
    m_connection->deleteLater();
    m_connection = nullptr;
}

void DebugSession::_stateChanged(KDevelop::IDebugSession::DebuggerState state)
{
    qCDebug(KDEV_PHP_DEBUGGER) << state;
    if (state == StartingState) {
        run();
    } else if (state == PausedState) {
        raiseEvent(program_state_changed);
    } else if (state == StoppingState) {
        m_connection->sendCommand("stop");
    } else if (state == EndedState) {
        raiseEvent(debugger_exited);
        emit finished();
    }
}

DebugSession::DebuggerState DebugSession::state() const
{
    if (!m_connection) {
        return NotStartedState;
    }
    return m_connection->currentState();
}

void DebugSession::run()
{
    Q_ASSERT(m_connection);
    m_connection->sendCommand("run");
    m_connection->setState(ActiveState);
}

void DebugSession::stepOut()
{
    Q_ASSERT(m_connection);
    m_connection->sendCommand("step_out");
    m_connection->setState(ActiveState);
}

void DebugSession::stepOverInstruction()
{
}

void DebugSession::stepInto()
{
    Q_ASSERT(m_connection);
    m_connection->sendCommand("step_into");
    m_connection->setState(ActiveState);
}

void DebugSession::stepIntoInstruction()
{
}

void DebugSession::stepOver()
{
    Q_ASSERT(m_connection);
    m_connection->sendCommand("step_over");
    m_connection->setState(ActiveState);
}

void DebugSession::jumpToCursor()
{
}

void DebugSession::runToCursor()
{
}

void DebugSession::interruptDebugger()
{
}

void DebugSession::stopDebugger()
{
    closeServer();
    // finish debugger when no connection active
    if ( state() == DebugSession::NotStartedState && !m_connection ) {
        stateChanged(DebugSession::EndedState);
        emit finished();
    } else {
        m_connection->sendCommand("stop");
    }
}

void DebugSession::killDebuggerNow()
{
    closeServer();
    if (m_connection) {
        m_connection->close();
    } else {
        emit stateChanged(EndedState);
        emit finished();
    }
}

void DebugSession::restartDebugger()
{
}
void DebugSession::eval(QByteArray source)
{
    Q_ASSERT(m_connection);
    m_connection->sendCommand("eval", QStringList(), source);
}

bool DebugSession::waitForFinished(int msecs)
{
    QTime stopWatch;
    stopWatch.start();
    if (!waitForState(DebugSession::StoppingState, msecs)) {
        return false;
    }
    if (msecs != -1) {
        msecs = msecs - stopWatch.elapsed();
    }
    return true;
}

bool DebugSession::waitForState(KDevelop::IDebugSession::DebuggerState state, int msecs)
{
    if (!m_connection) {
        return false;
    }
    if (m_connection->currentState() == state) {
        return true;
    }
    QTime stopWatch;
    stopWatch.start();
    if (!waitForConnected(msecs)) {
        return false;
    }
    while (m_connection->currentState() != state) {
        if (!m_connection) {
            return false;
        }
        if (!m_connection->socket()) {
            return false;
        }
        if (!m_connection->socket()->isOpen()) {
            return false;
        }
        m_connection->socket()->waitForReadyRead(100);
        if (msecs != -1 && stopWatch.elapsed() > msecs) {
            return false;
        }
    }
    return true;
}

bool DebugSession::waitForConnected(int msecs)
{
    if (!m_connection) {
        Q_ASSERT(m_server);
        if (!m_server->waitForNewConnection(msecs)) {
            return false;
        }
    }
    Q_ASSERT(m_connection);
    Q_ASSERT(m_connection->socket());
    return m_connection->socket()->waitForConnected(msecs);
}

Connection* DebugSession::connection()
{
    return m_connection;
}

bool DebugSession::restartAvaliable() const
{
    //not supported at all by xdebug
    return false;
}

KDevelop::IBreakpointController* DebugSession::breakpointController() const
{
    return m_breakpointController;
}

KDevelop::IVariableController* DebugSession::variableController() const
{
    return m_variableController;
}

KDevelop::IFrameStackModel* DebugSession::frameStackModel() const
{
    return m_frameStackModel;
}

QPair<QUrl, int> DebugSession::convertToLocalUrl(const QPair<QUrl, int>& remoteUrl) const
{
    Q_ASSERT(m_launchConfiguration);
    QPair<QUrl, int> ret = remoteUrl;
    ret.first = KDevelop::PathMappings::convertToLocalUrl(m_launchConfiguration->config(), remoteUrl.first);
    return ret;
}

QPair<QUrl, int> DebugSession::convertToRemoteUrl(const QPair<QUrl, int>& localUrl) const
{
    Q_ASSERT(m_launchConfiguration);
    QPair<QUrl, int> ret = localUrl;
    ret.first = KDevelop::PathMappings::convertToRemoteUrl(m_launchConfiguration->config(), localUrl.first);
    return ret;
}

void DebugSession::currentPositionChanged(const QUrl& url, int line)
{
    setCurrentPosition(url, line, QString());
}
}
