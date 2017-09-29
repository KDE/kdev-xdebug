/*
 * XDebug-specific implementation of thread and frame model.
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

#include <QXmlStreamReader>

#include <QDebug>

#include "framestackmodel.h"
#include "connection.h"
#include <QDomElement>

namespace XDebug {
void FrameStackModel::fetchThreads()
{
    //no multithreading in php, create just one
    QList<KDevelop::FrameStackModel::ThreadItem> threadsList;
    KDevelop::FrameStackModel::ThreadItem i;
    i.nr = 0;
    i.name = "main thread";
    threadsList << i;
    setThreads(threadsList);
    setCurrentThread(0);
}

void FrameStackModel::handleStack(const QDomDocument& xml)
{
    Q_ASSERT(xml.documentElement().attribute("command") == "stack_get");

    QList<KDevelop::FrameStackModel::FrameItem> frames;
    QDomElement el = xml.documentElement().firstChildElement("stack");
    while (!el.isNull()) {
        KDevelop::FrameStackModel::FrameItem f;
        f.nr = el.attribute("level").toInt();
        f.name = el.attribute("where");
        f.file = el.attribute("filename");
        f.line = el.attribute("lineno").toInt() - 1;
        frames << f;
        el = el.nextSiblingElement("stack");
    }
    setFrames(0, frames);
    setHasMoreFrames(0, false);
}

void FrameStackModel::fetchFrames(int threadNumber, int from, int to)
{
    Q_UNUSED(from); //we fetch always everything
    Q_UNUSED(to);

    if (threadNumber == 0) { //we support only one thread
        Callback<FrameStackModel>* cb = new Callback<FrameStackModel>(this, &FrameStackModel::handleStack);
        session()->connection()->sendCommand("stack_get", QStringList(), QByteArray(), cb);
    }
}
}
