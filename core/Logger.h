/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_CLOGGER_H_
#define _INCLUDE_SOURCEMOD_CLOGGER_H_

#include <sh_string.h>
#include "sm_globals.h"

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
	LoggingMode_HL2
};

class Logger : public SMGlobalClass
{
public:
	Logger() : m_ErrMapStart(false), m_Active(false), m_DelayedStart(false), m_DailyPrintHdr(false) {}
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllShutdown();
public:
	void InitLogger(LoggingMode mode, bool startlogging);
	void CloseLogger();
	void EnableLogging();
	void DisableLogging();
	void LogMessage(const char *msg, ...);
	void LogError(const char *msg, ...);
	void MapChange(const char *mapname);
	const char *GetLogFileName(LogType type) const;
	LoggingMode GetLoggingMode() const;
private:
	void _CloseFile();
	void _NewMapFile();
	void _PrintToHL2Log(const char *fmt, va_list ap);
private:
	String m_NrmFileName;
	String m_ErrFileName;
	String m_CurMapName;
	LoggingMode m_mode;
	int m_CurDay;
	bool m_ErrMapStart;
	bool m_Active;
	bool m_DelayedStart;
	bool m_DailyPrintHdr;
};

extern Logger g_Logger;

#endif // _INCLUDE_SOURCEMOD_CLOGGER_H_
