// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#include "vprof_tool.h"
#include "logic_bridge.h"
#include "sourcemod.h"
#include "sourcemm_api.h"

#define VPROF_ENABLED
#define RAD_TELEMETRY_DISABLED
#include <tier0/vprof.h>

VProfTool sVProfTool;

VProfTool::VProfTool()
	: active_(false)
{
}

void
VProfTool::OnSourceModAllInitialized()
{
	logicore.RegisterProfiler(this);
}

const char *
VProfTool::Name()
{
	return "vprof";
}

const char *
VProfTool::Description()
{
	return "Valve built-in profiler";
}

bool
VProfTool::Start()
{
	g_VProfCurrentProfile.Start();
	return IsActive();
}

void
VProfTool::Stop(void (*render)(const char *fmt, ...))
{
	g_VProfCurrentProfile.Stop();
	RenderHelp(render);
}

void
VProfTool::Dump()
{
	g_VProfCurrentProfile.Pause();
	g_VProfCurrentProfile.OutputReport(VPRT_FULL);
	g_VProfCurrentProfile.Resume();
}

bool
VProfTool::IsActive()
{
	return g_VProfCurrentProfile.IsEnabled();
}

bool
VProfTool::IsAttached()
{
	return true;
}

void
VProfTool::EnterScope(const char *group, const char *name)
{
	if (IsActive()) {
		if (!group)
			group = VPROF_BUDGETGROUP_OTHER_UNACCOUNTED;
		g_VProfCurrentProfile.EnterScope(name, 1, group, false, 0);
	}
}

void 
VProfTool::LeaveScope()
{
	if (IsActive())
		g_VProfCurrentProfile.ExitScope();
}

void
VProfTool::RenderHelp(void (*render)(const char *fmt, ...))
{
	render("Use 'sm prof dump vprof' or one of the vprof_generate_report commands in your console to analyze a profile session.");
}
