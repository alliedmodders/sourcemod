/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_PLUGIN_PROFILER_H_
#define _INCLUDE_SOURCEMOD_PLUGIN_PROFILER_H_

#include <sp_vm_api.h>
#include <sm_platform.h>
#include <sm_trie_tpl.h>
#include <sh_vector.h>
#include <sh_stack.h>
#include <stdio.h>
#include "common_logic.h"
#include <IRootConsoleMenu.h>

using namespace SourcePawn;
using namespace SourceHook;

struct prof_point_t
{
#if defined PLATFORM_WINDOWS
	LARGE_INTEGER value;
#elif defined PLATFORM_POSIX
	struct timeval value;
#endif
	bool is_set;
};

struct prof_atom_t
{
	int atom_type;			/* Type of object we're profiling */
	int atom_serial;		/* Serial number, if appropriate */
	sp_context_t *ctx;		/* Plugin context. */
	const char *name;		/* Name of the function */
	prof_point_t start;		/* Start time */
	prof_point_t end;		/* End time */
	double base_time;		/* Known time from children or pausing. */
};

struct prof_atom_report_t
{
	char atom_name[256];	/* Full name to shove to logs */
	double total_time;		/* Total time spent executing, in s */
	unsigned int num_calls;	/* Number of invocations */
	double min_time;		/* Min time spent in one call, in s */
	double max_time;		/* Max time spent in one call, in s */
};

class ProfileReport
{
public:
	~ProfileReport();
public:
	void SaveAtom(const prof_atom_t &atom);
	size_t GetNumReports();
	prof_atom_report_t *GetReport(size_t i);
	void Clear();
private:
	KTrie<prof_atom_report_t *> m_ReportLookup;
	CVector<prof_atom_report_t *> m_Reports;
};

class ProfileEngine :
	public SMGlobalClass,
	public IRootConsoleCommand,
	public IProfiler
{
public:
	ProfileEngine();
public:
	bool IsEnabled();
	bool GenerateReport(FILE *fp);
	void Clear();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IRootConsoleCommand
	void OnRootConsoleCommand2(const char *cmdname, const ICommandArgs *command);
public: //IProfiler
	void OnNativeBegin(IPluginContext *pContext, sp_native_t *native);
	void OnNativeEnd() ;
	void OnFunctionBegin(IPluginContext *pContext, const char *name);
	void OnFunctionEnd();
	int OnCallbackBegin(IPluginContext *pContext, sp_public_t *pubfunc);
	void OnCallbackEnd(int serial);
private:
	void PushProfileStack(IPluginContext *ctx, int type, const char *name);
	void PopProfileStack(ProfileReport *reporter);
	void PauseParent();
	void ResumeParent(double addTime);
	void WriteReport(FILE *fp, ProfileReport *report, const char *name);
private:
	CStack<prof_atom_t> m_AtomStack;
	ProfileReport m_Callbacks;
	ProfileReport m_Functions;
	ProfileReport m_Natives;
	int m_serial;
	prof_point_t m_ProfStart;
};

extern ProfileEngine g_Profiler;

#endif //_INCLUDE_SOURCEMOD_PLUGIN_PROFILER_H_
