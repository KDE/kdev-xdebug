/*
   Copyright 2009 Niko Sams <niko.sams@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "breakpointcontroller.h"

#include <QDomElement>
#include <KDebug>
#include <KLocalizedString>

#include <debugger/breakpoint/breakpoint.h>

#include "debugsession.h"
#include "connection.h"

namespace XDebug {

BreakpointController::BreakpointController(DebugSession* parent)
    : IBreakpointController(parent)
{
    connect(debugSession(), SIGNAL(stateChanged(KDevelop::IDebugSession::DebuggerState)),
                SLOT(stateChanged(KDevelop::IDebugSession::DebuggerState)));
}

void BreakpointController::sendMaybe(KDevelop::Breakpoint* breakpoint)
{
    if (!debugSession()->connection()) return;

    if (breakpoint->deleted()) {
        if (m_ids.contains(breakpoint)) {
            QString cmd("breakpoint_remove");
            QStringList args;
            args << "-d "+m_ids[breakpoint];
            debugSession()->connection()->sendCommand(cmd, args);
        }
    } else if (m_dirty[breakpoint].contains(KDevelop::Breakpoint::LocationColumn)) {
        if (breakpoint->enabled()) {
            QString cmd = m_ids.contains(breakpoint) ? "breakpoint_update" : "breakpoint_set";
            QStringList args;
            kDebug() << "breakpoint kind" << breakpoint->kind();
            if (breakpoint->kind() == KDevelop::Breakpoint::CodeBreakpoint) {
                if (m_ids.contains(breakpoint)) {
                    args << "-d "+m_ids[breakpoint];
                } else if (breakpoint->line() != -1) {
                    args << "-t line";
                } else {
                    args << "-t call";
                }
                if (breakpoint->line() != -1) {
                    QPair<KUrl, int> u = qMakePair(breakpoint->url(), breakpoint->line());
                    u = debugSession()->convertToRemoteUrl(u);
                    args << "-f "+u.first.url();
                    args << "-n "+QString::number(u.second+1);
                } else {
                    args << "-m "+breakpoint->expression();
                }
            } else if (breakpoint->kind() == KDevelop::Breakpoint::WriteBreakpoint) {
                args << "-t watch";
                args << "-m "+breakpoint->expression();
            } else {
                error(breakpoint, i18n("breakpoint type is not supported"), KDevelop::Breakpoint::LocationColumn);
                return;
            }
            if (breakpoint->ignoreHits()) {
                args << "-h "+QString::number(breakpoint->ignoreHits()+1);
                args << "-o >=";
            }
            CallbackWithCookie<BreakpointController, KDevelop::Breakpoint>* cb =
                new CallbackWithCookie<BreakpointController, KDevelop::Breakpoint>
                    (this, &BreakpointController::handleSetBreakpoint, breakpoint, true);
            debugSession()->connection()->sendCommand(cmd, args, breakpoint->condition().toUtf8(), cb);
        }
    } else if (m_dirty[breakpoint].contains(KDevelop::Breakpoint::EnableColumn)) {
        Q_ASSERT(m_ids.contains(breakpoint));
        QString cmd = "breakpoint_update";
        QStringList args;
        args << "-d "+m_ids[breakpoint];
        args << QString("-s %0").arg(breakpoint->enabled() ? "enabled" : "disabled");
        debugSession()->connection()->sendCommand(cmd, args);
    }
    m_dirty[breakpoint].clear();
}

void BreakpointController::handleSetBreakpoint(KDevelop::Breakpoint* breakpoint, const QDomDocument &xml)
{
    Q_ASSERT(xml.documentElement().attribute("command") == "breakpoint_set"
        || xml.documentElement().attribute("command") == "breakpoint_update");
    Q_ASSERT(breakpoint);
    if (xml.documentElement().attribute("command") == "breakpoint_set") {
        m_ids[breakpoint] = xml.documentElement().attribute("id");
    }
    if (!xml.documentElement().firstChildElement("error").isNull()) {
        kWarning() << "breakpoint error" << xml.documentElement().firstChildElement("error").text();
        error(breakpoint, xml.documentElement().firstChildElement("error").text(), KDevelop::Breakpoint::LocationColumn);
    }
}

DebugSession* BreakpointController::debugSession()
{
    return static_cast<DebugSession*>(KDevelop::IBreakpointController::debugSession());
}

void BreakpointController::stateChanged(KDevelop::IDebugSession::DebuggerState state)
{
    kDebug() << state;
    if (state == KDevelop::IDebugSession::StartingState) {
        m_ids.clear();
        sendMaybeAll();
    } else if (state == KDevelop::IDebugSession::PausedState) {
        Callback<BreakpointController>* cb =
            new Callback<BreakpointController>(this, &BreakpointController::handleBreakpointList);
        debugSession()->connection()->sendCommand("breakpoint_list", QStringList(), QByteArray(), cb);
    }
}

void BreakpointController::handleBreakpointList(const QDomDocument &xml)
{
    Q_ASSERT(xml.firstChildElement().attribute("command") == "breakpoint_list");

    QDomElement el = xml.documentElement().firstChildElement("breakpoint");
    while (!el.isNull()) {
        KDevelop::Breakpoint *b = m_ids.key(el.attribute("id"));
        setHitCount(b, el.attribute("hit_count").toInt());
        el = el.nextSiblingElement("breakpoint");
    }

}

}

#include "breakpointcontroller.moc"
