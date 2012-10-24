/*
* XDebug Debugger Support
*
* Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
* Copyright 2007 Hamish Rodda <rodda@kde.org>
* Copyright 2009 Andreas Pakulat <apaku@gmx.de>
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
#ifndef XDEBUGDEBUGJOB
#define XDEBUGDEBUGJOB

#include <QProcess>
#include <KUrl>

#include <outputview/outputjob.h>

class KProcess;
namespace KDevelop
{
class OutputModel;
class ILaunchConfiguration;
class ProcessLineMaker;
}

namespace XDebug
{

class DebugSession;

class XDebugJob : public KDevelop::OutputJob
{
Q_OBJECT
public:
    XDebugJob( DebugSession* session, KDevelop::ILaunchConfiguration*, QObject* parent = 0 );
    virtual void start();

    KProcess *process() const;
protected:
    virtual bool doKill();

private:
    KDevelop::OutputModel* model();

private slots:
    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);
private:
    void appendLine(const QString &l);
    KProcess* m_proc;
    KDevelop::ProcessLineMaker* m_lineMaker;
    QPointer<DebugSession> m_session;
};

class XDebugBrowserJob : public KJob
{
    Q_OBJECT
public:
    XDebugBrowserJob( DebugSession* session, KDevelop::ILaunchConfiguration*, QObject* parent = 0 );
    virtual void start();
protected:
    virtual bool doKill();

private slots:
    void sessionFinished();

private:
    KUrl m_url;
    QString m_browser;
    DebugSession* m_session;
};


}

#endif
