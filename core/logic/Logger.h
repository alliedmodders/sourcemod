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

#ifndef _INCLUDE_SOURCEMOD_CLOGGER_H_
#define _INCLUDE_SOURCEMOD_CLOGGER_H_

#include "common_logic.h"
#include <stdio.h>
#include <sh_string.h>
#include <bridge/include/ILogger.h>

using namespace SourceHook;

enum LogType
{
	LogType_Normal,
	LogType_Error
};

enum LoggingMode
{
	LoggingMode_Daily,
	LoggingMode_PerMap,
	LoggingMode_Game
};

class Logger : public SMGlobalClass, public ILogger
{
public:
	Logger() : m_Mode(LoggingMode_Daily), m_ErrMapStart(false), 
		m_Active(false), m_DelayedStart(false), m_DailyPrintHdr(false), 
		m_InitialState(true)
	{
	}
public: //SMGlobalClass
	ConfigResult OnSourceModConfigChanged(const char *key, 
		const char *value, 
		ConfigSource source,
		char *error, 
		size_t maxlength);
	void OnSourceModStartup(bool late);
	void OnSourceModAllShutdown();
	void OnSourceModLevelChange(const char *mapName);
public:
	void InitLogger(LoggingMode mode);
	void CloseLogger();
	void EnableLogging();
	void DisableLogging();
	void LogMessage(const char *msg, ...);
	void LogMessageEx(const char *msg, va_list ap);
	void LogError(const char *msg, ...);
	void LogErrorEx(const char *msg, va_list ap); 
	void LogFatal(const char *msg, ...);
	void LogFatalEx(const char *msg, va_list ap);
	void LogToOpenFile(FILE *fp, const char *msg, ...);
	void LogToOpenFileEx(FILE *fp, const char *msg, va_list ap);
	/* This version does not print to console, and is thus thread-safe */
	void LogToFileOnly(FILE *fp, const char *msg, ...);
	void LogToFileOnlyEx(FILE *fp, const char *msg, va_list ap);
	void MapChange(const char *mapname);
	const char *GetLogFileName(LogType type) const;
	LoggingMode GetLoggingMode() const;
private:
	void _CloseFile();
	void _NewMapFile();
	void _PrintToGameLog(const char *fmt, va_list ap);
private:
	String m_NrmFileName;
	String m_ErrFileName;
	String m_CurMapName;
	LoggingMode m_Mode;
	int m_NrmCurDay;
	int m_ErrCurDay;
	bool m_ErrMapStart;
	bool m_Active;
	bool m_DelayedStart;
	bool m_DailyPrintHdr;
	bool m_InitialState;
};

extern Logger g_Logger;

#endif // _INCLUDE_SOURCEMOD_CLOGGER_H_
