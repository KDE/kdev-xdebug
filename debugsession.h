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

#ifndef XDEBUG_DEBUGSESSION_H
#define XDEBUG_DEBUGSESSION_H

#include <QString>
#include <debugger/interfaces/idebugsession.h>

namespace KDevelop {
class ILaunchConfiguration;
}
class KJob;
class QTcpServer;
class KProcess;

namespace XDebug {
class BreakpointController;
class FrameStackModel;
class VariableController;
class Connection;

class DebugSession
    : public KDevelop::IDebugSession
{
    Q_OBJECT

public:
    DebugSession();
    ~DebugSession() override;

    void setLaunchConfiguration(KDevelop::ILaunchConfiguration* cfg);
    void setAcceptMultipleConnections(bool v);

    bool listenForConnection(QString& error);

    bool waitForState(DebuggerState state, int msecs = 30000);
    bool waitForFinished(int msecs = 30000);
    bool waitForConnected(int msecs = 30000);

    Connection* connection();

    DebuggerState state() const override;

    bool restartAvaliable() const override;

    QPair<QUrl, int> convertToLocalUrl(const QPair<QUrl, int>& url) const override;
    QPair<QUrl, int> convertToRemoteUrl(const QPair<QUrl, int>& url) const override;

    KDevelop::IBreakpointController* breakpointController() const override;
    KDevelop::IVariableController* variableController() const override;
    KDevelop::IFrameStackModel* frameStackModel() const override;

Q_SIGNALS:
    void output(QString line);
    void outputLine(QString line);

public Q_SLOTS:
    void run() override;
    void stepOut() override;
    void stepOverInstruction() override;
    void stepInto() override;
    void stepIntoInstruction() override;
    void stepOver() override;
    void jumpToCursor() override;
    void runToCursor() override;
    void interruptDebugger() override;
    void stopDebugger() override;
    void restartDebugger() override;

    void eval(QByteArray source);

private Q_SLOTS:
    void incomingConnection();
    void _stateChanged(KDevelop::IDebugSession::DebuggerState);
    void connectionClosed();
    void currentPositionChanged(const QUrl& url, int line);

private:
    void closeServer();

private:
    BreakpointController* m_breakpointController;
    VariableController* m_variableController;
    FrameStackModel* m_frameStackModel;

    QTcpServer* m_server;
    Connection* m_connection;
    KDevelop::ILaunchConfiguration* m_launchConfiguration;
    bool m_acceptMultipleConnections;
};
}

#endif // XDEBUG_DEBUGSESSION_H
