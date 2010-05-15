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

#include "Profiler.h"
#include <ISourceMod.h>
#if defined PLATFORM_POSIX
#include <sys/time.h>
#include <time.h>
#endif
#include <IPluginSys.h>
#include "stringutil.h"

ProfileEngine g_Profiler;
IProfiler *sm_profiler = &g_Profiler;

#if defined PLATFORM_WINDOWS
double WINDOWS_PERFORMANCE_FREQUENCY;
#endif

class EmptyProfiler : public IProfiler
{
public:
	void OnNativeBegin(IPluginContext *pContext, sp_native_t *native)
	{
	}
	void OnNativeEnd()
	{
	}
	void OnFunctionBegin(IPluginContext *pContext, const char *name)
	{
	}
	void OnFunctionEnd()
	{
	}
	int OnCallbackBegin(IPluginContext *pContext, sp_public_t *pubfunc)
	{
		return 0;
	}
	void OnCallbackEnd(int serial)
	{
	}
} s_EmptyProfiler;

inline void InitProfPoint(prof_point_t &pt)
{
#if defined PLATFORM_WINDOWS
	QueryPerformanceCounter(&pt.value);
#elif defined PLATFORM_POSIX
	gettimeofday(&pt.value, NULL);
#endif
	pt.is_set = true;
}

ProfileEngine::ProfileEngine()
{
	m_serial = 0;

#if defined PLATFORM_WINDOWS
	LARGE_INTEGER pf;

	if (QueryPerformanceFrequency(&pf))
	{
		WINDOWS_PERFORMANCE_FREQUENCY = 1.0 / (double)(pf.QuadPart);
	}
	else
	{
		WINDOWS_PERFORMANCE_FREQUENCY = -1.0;
	}
#endif

	if (IsEnabled())
	{
		InitProfPoint(m_ProfStart);
	}
	else
	{
		sm_profiler = &s_EmptyProfiler;
	}
}

bool ProfileEngine::IsEnabled()
{
#if defined PLATFORM_WINDOWS
	return (WINDOWS_PERFORMANCE_FREQUENCY > 0.0);
#elif defined PLATFORM_POSIX
	return true;
#endif
}

inline double DiffProfPoints(const prof_point_t &start, const prof_point_t &end)
{
	double seconds;

#if defined PLATFORM_WINDOWS
	LONGLONG diff;
	
	diff = end.value.QuadPart - start.value.QuadPart;
	seconds = diff * WINDOWS_PERFORMANCE_FREQUENCY;
#elif defined PLATFORM_POSIX
	seconds = (double)(end.value.tv_sec - start.value.tv_sec);

	if (start.value.tv_usec > end.value.tv_usec)
	{
		seconds -= 1.0;
		seconds += (double)(1000000 - (start.value.tv_usec - end.value.tv_usec)) / 1000000.0;
	}
	else
	{
		seconds += (double)(end.value.tv_usec - start.value.tv_usec) / 1000000.0;
	}
#endif

	return seconds;
}

inline double CalcAtomTime(const prof_atom_t &atom)
{
	if (!atom.end.is_set)
	{
		return atom.base_time;
	}

	return atom.base_time + DiffProfPoints(atom.start, atom.end);
}

void ProfileEngine::OnNativeBegin(IPluginContext *pContext, sp_native_t *native)
{
	PushProfileStack(pContext, SP_PROF_NATIVES, native->name);
}

void ProfileEngine::OnNativeEnd()
{
	assert(!m_AtomStack.empty());
	assert(m_AtomStack.front().atom_type == SP_PROF_NATIVES);

	PopProfileStack(&m_Natives);
}

void ProfileEngine::OnFunctionBegin(IPluginContext *pContext, const char *name)
{
	PushProfileStack(pContext, SP_PROF_FUNCTIONS, name);
}

void ProfileEngine::OnFunctionEnd()
{
	assert(!m_AtomStack.empty());
	assert(m_AtomStack.front().atom_type == SP_PROF_FUNCTIONS);

	PopProfileStack(&m_Functions);
}

int ProfileEngine::OnCallbackBegin(IPluginContext *pContext, sp_public_t *pubfunc)
{
	PushProfileStack(pContext, SP_PROF_CALLBACKS, pubfunc->name);

	return m_serial;
}

void ProfileEngine::OnCallbackEnd(int serial)
{
	assert(!m_AtomStack.empty());

	/**
	 * Account for the situation where the JIT discards the 
	 * stack because there was an RTE of sorts.
	 */
	if (m_AtomStack.front().atom_type != SP_PROF_CALLBACKS
		&& m_AtomStack.front().atom_serial != serial)
	{
		prof_atom_t atom;
		double total_time;

		/* There was an error, and we need to discard things. */
		total_time = 0.0;
		while (!m_AtomStack.empty() 
			&& m_AtomStack.front().atom_type != SP_PROF_CALLBACKS 
			&& m_AtomStack.front().atom_serial != serial)
		{
			total_time += CalcAtomTime(m_AtomStack.front());
			m_AtomStack.pop();
		}

		/**
		 * Now we can end and discard ourselves, without saving the data.
		 * Since this data is all erroneous anyway, we don't care if it's 
		 * not totally accurate. 
		 */

		assert(!m_AtomStack.empty());
		atom = m_AtomStack.front();
		m_AtomStack.pop();

		/* Note: We don't need to resume ourselves because end is set by Pause(). */
		total_time += CalcAtomTime(atom);

		ResumeParent(total_time);
		return;
	}

	PopProfileStack(&m_Callbacks);
}

void ProfileEngine::PushProfileStack(IPluginContext *ctx, int type, const char *name)
{
	prof_atom_t atom;

	PauseParent();

	atom.atom_type = type;
	atom.base_time = 0.0;
	atom.ctx = ctx->GetContext();
	atom.name = name;
	atom.end.is_set = false;

	if (type == SP_PROF_CALLBACKS)
	{
		atom.atom_serial = ++m_serial;
	}
	else
	{
		atom.atom_serial = 0;
	}

	m_AtomStack.push(atom);

	/* Note: We do this after because the stack could grow and skew results */
	InitProfPoint(m_AtomStack.front().start);
}

void ProfileEngine::PopProfileStack(ProfileReport *reporter)
{
	double total_time;

	prof_atom_t &atom = m_AtomStack.front();

	/* We're okay to cache our used time. */
	InitProfPoint(atom.end);
	total_time = CalcAtomTime(atom);

	/* Now it's time to save this! This may do a lot of computations which 
	 * is why we've cached the time beforehand.
	 */
	reporter->SaveAtom(atom);
	m_AtomStack.pop();

	/* Finally, tell our parent how much time we used. */
	ResumeParent(total_time);
}

void ProfileEngine::PauseParent()
{
	if (m_AtomStack.empty())
	{
		return;
	}

	InitProfPoint(m_AtomStack.front().end);
}

void ProfileEngine::ResumeParent(double addTime)
{
	if (m_AtomStack.empty())
	{
		return;
	}

	prof_atom_t &atom = m_AtomStack.front();

	/* Move its "paused time" to its base (known) time, 
	 * then reset the start/end.  Note that since CalcAtomTime() 
	 * reads the base time, we SHOULD NOT use += to add.
	 */
	atom.base_time = CalcAtomTime(atom);
	atom.base_time += addTime;
	InitProfPoint(atom.start);
	atom.end.is_set = false;
}

void ProfileEngine::Clear()
{
	m_Natives.Clear();
	m_Callbacks.Clear();
	m_Functions.Clear();
	InitProfPoint(m_ProfStart);
}

void ProfileEngine::OnSourceModAllInitialized()
{
	rootmenu->AddRootConsoleCommand2("profiler", "Profiler commands", this);
}

void ProfileEngine::OnSourceModShutdown()
{
	rootmenu->RemoveRootConsoleCommand("profiler", this);
}

void ProfileEngine::OnRootConsoleCommand2(const char *cmdname, const ICommandArgs *command)
{
	if (command->ArgC() >= 3)
	{
		if (strcmp(command->Arg(2), "flush") == 0)
		{
			FILE *fp;
			char path[256];

			g_pSM->BuildPath(Path_SM, path, sizeof(path), "logs/profile_%d.xml", (int)time(NULL));

			if ((fp = fopen(path, "wt")) == NULL)
			{
				rootmenu->ConsolePrint("Failed, could not open file for writing: %s", path);
				return;
			}

			GenerateReport(fp);

			fclose(fp);

			rootmenu->ConsolePrint("Profiler report generated as: %s\n", path);

			return;
		}
	}

	rootmenu->ConsolePrint("Profiler commands:");
	rootmenu->DrawGenericOption("flush", "Flushes statistics to disk and starts over");
}

bool ProfileEngine::GenerateReport(FILE *fp)
{
	time_t t;
	double total_time;
	prof_point_t end_time;

	InitProfPoint(end_time);
	total_time = DiffProfPoints(m_ProfStart, end_time);

	t = time(NULL);

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
	fprintf(fp, "<profile time=\"%d\" uptime=\"%f\">\n", (int)t, total_time);
	WriteReport(fp, &m_Natives, "natives");
	WriteReport(fp, &m_Callbacks, "callbacks");
	WriteReport(fp, &m_Functions, "functions");
	fprintf(fp, "</profile>\n");

	return true;
}

void ProfileEngine::WriteReport(FILE *fp, ProfileReport *report, const char *name)
{
	size_t i, num;
	prof_atom_report_t *ar;
	char new_name[512];

	fprintf(fp, " <report name=\"%s\">\n", name);
	
	num = report->GetNumReports();
	for (i = 0; i < num; i++)
	{
		ar = report->GetReport(i);

		strncopy(new_name, ar->atom_name, sizeof(new_name));
		UTIL_ReplaceAll(new_name, sizeof(new_name), "<", "&lt;", true);
		UTIL_ReplaceAll(new_name, sizeof(new_name), ">", "&gt;", true);

		fprintf(fp, "  <item name=\"%s\" numcalls=\"%d\" mintime=\"%f\" maxtime=\"%f\" totaltime=\"%f\"/>\n", 
			new_name,
			ar->num_calls,
			ar->min_time,
			ar->max_time,
			ar->total_time);
	}

	fprintf(fp, " </report>\n");
}

ProfileReport::~ProfileReport()
{
	for (size_t i = 0; i < m_Reports.size(); i++)
	{
		delete m_Reports[i];
	}
}

void ProfileReport::Clear()
{
	m_ReportLookup.clear();
	for (size_t i = 0; i < m_Reports.size(); i++)
	{
		delete m_Reports[i];
	}
	m_Reports.clear();
}

size_t ProfileReport::GetNumReports()
{
	return m_Reports.size();
}

prof_atom_report_t *ProfileReport::GetReport(size_t i)
{
	return m_Reports[i];
}

void ProfileReport::SaveAtom(const prof_atom_t &atom)
{
	double atom_time;
	char full_name[256];
	prof_atom_report_t **pReport, *report;

	if (atom.atom_type == SP_PROF_NATIVES)
	{
		smcore.strncopy(full_name, atom.name, sizeof(full_name));
	}
	else
	{
		IPlugin *pl;
		const char *file;

		file = "unknown";
		if ((pl = pluginsys->FindPluginByContext(atom.ctx)) != NULL)
		{
			file = pl->GetFilename();
		}

		smcore.Format(full_name, sizeof(full_name), "%s!%s", file, atom.name);
	}

	atom_time = CalcAtomTime(atom);

	if ((pReport = m_ReportLookup.retrieve(full_name)) == NULL)
	{
		report = new prof_atom_report_t;

		smcore.strncopy(report->atom_name, full_name, sizeof(report->atom_name));
		report->max_time = atom_time;
		report->min_time = atom_time;
		report->num_calls = 1;
		report->total_time = atom_time;
		
		m_ReportLookup.insert(full_name, report);
		m_Reports.push_back(report);
	}
	else
	{
		report = *pReport;

		if (atom_time > report->max_time)
		{
			report->max_time = atom_time;
		}
		if (atom_time < report->min_time)
		{
			report->min_time = atom_time;
		}
		report->num_calls++;
		report->total_time += atom_time;
	}
}
