/*
 * XDebug-specific implementation of thread and frame model.
 *
 * Copyright 2009 Niko Sams <nikol.sams@gmail.com>
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

#ifndef XDEBUG_FRAMESTACKMODEL_H
#define XDEBUG_FRAMESTACKMODEL_H

#include <debugger/framestack/framestackmodel.h>

#include "debugsession.h"

class QDomDocument;
namespace XDebug {

class FrameStackModel : public KDevelop::FrameStackModel
{
public:
    FrameStackModel(DebugSession* session) : KDevelop::FrameStackModel(session) {}

public:
    DebugSession* session() { return static_cast<DebugSession*>(KDevelop::FrameStackModel::session()); }

protected: // KDevelop::FrameStackModel overrides
    virtual void fetchThreads();
    virtual void fetchFrames(int threadNumber, int from, int to);

private:
    void handleStack(const QDomDocument &xml);
};

}

#endif
