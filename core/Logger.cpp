/**
 * vim: set ts=4 :
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

#include <time.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "Logger.h"
#include "systems/LibrarySys.h"
#include "sm_version.h"

Logger g_Logger;

/**
 * :TODO: This should be creating the log folder if it doesn't exist
 */

ConfigResult Logger::OnSourceModConfigChanged(const char *key, 
									  const char *value, 
									  ConfigSource source,
									  char *error, 
									  size_t maxlength)
{
	if (strcasecmp(key, "Logging") == 0)
	{
		bool state;

		if (strcasecmp(value, "on") == 0)
		{
			state = true;
		} else if (strcasecmp(value, "off") == 0) {
			state = false;
		} else {
			UTIL_Format(error, maxlength, "Invalid value: must be \"on\" or \"off\"");
			return ConfigResult_Reject;
		}

		if (source == ConfigSource_Console)
		{
			state ? EnableLogging() : DisableLogging();
		} else {
			m_InitialState = state;
		}

		return ConfigResult_Accept;
	} else if (strcasecmp(key, "LogMode") == 0) {
		if (strcasecmp(value, "daily") == 0) 
		{
			m_Mode = LoggingMode_Daily;
		} else if (strcasecmp(value, "map") == 0) {
			m_Mode = LoggingMode_PerMap;
		} else if (strcasecmp(value, "game") == 0) {
			m_Mode = LoggingMode_Game;
		} else {
			UTIL_Format(error, maxlength, "Invalid value: must be [daily|map|game]");
			return ConfigResult_Reject;
		}

		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void Logger::OnSourceModStartup(bool late)
{
	InitLogger(m_Mode);
}

void Logger::OnSourceModAllShutdown()
{
	CloseLogger();
}

void Logger::_NewMapFile()
{
	if (!m_Active)
	{
		return;
	}

	/* Append "Log file closed" to previous log file */
	_CloseFile();
	
	char _filename[256];
	int i = 0;

	time_t t;
	time(&t);
	tm *curtime = localtime(&t);

	while (true)
	{
		g_SourceMod.BuildPath(Path_SM, _filename, sizeof(_filename), "logs/logs_%02d%02d%03d.log", curtime->tm_mon + 1, curtime->tm_mday, i);
		FILE *fp = fopen(_filename, "r");
		if (!fp)
		{
			break;
		}
		fclose(fp);
		i++;
	}
	m_NrmFileName.assign(_filename);

	FILE *fp = fopen(m_NrmFileName.c_str(), "w");
	if (!fp)
	{
		g_SMAPI->ConPrint("[SM] Unexpected fatal logging error. SourceMod logging disabled.\n");
		m_Active = false;
		return;
	} else {
		char date[32];
		strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
		fprintf(fp, "L %s: SourceMod log file started (file \"logs_%02d%02d%03d.log\") (Version \"%s\")\n", date, curtime->tm_mon + 1, curtime->tm_mday, i, SVN_FULL_VERSION);
		fclose(fp);
	}
}

void Logger::_CloseFile()
{
	if (!m_Active)
	{
		return;
	}

	FILE *fp = NULL;
	if (!m_NrmFileName.empty())
	{
		fp = fopen(m_NrmFileName.c_str(), "r+");
		if (fp)
		{
			fseek(fp, 0, SEEK_END);
			LogMessage("Log file closed.");
			fclose(fp);
		}
		m_NrmFileName.clear();
	}

	if (!m_ErrMapStart)
	{
		return;
	}
	fp = fopen(m_ErrFileName.c_str(), "r+");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		LogError("Error log file session closed.");
		fclose(fp);
	}
	m_ErrFileName.clear();
}

void Logger::InitLogger(LoggingMode mode)
{
	m_Mode = mode;
	m_Active = m_InitialState;

	time_t t;
	time(&t);
	tm *curtime = localtime(&t);
	m_CurDay = curtime->tm_mday;

	char _filename[256];
	g_SourceMod.BuildPath(Path_SM, _filename, sizeof(_filename), "logs/errors_%02d%02d%02d.log", curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_year - 100);
	m_ErrFileName.assign(_filename);

	switch (m_Mode)
	{
	case LoggingMode_PerMap:
		{
			if (!m_Active)
			{
				m_DelayedStart = true;
			}
			break;
		}
	case LoggingMode_Daily:
		{
			g_SourceMod.BuildPath(Path_SM, _filename, sizeof(_filename), "logs/logs_%02d%02d.log", curtime->tm_mon + 1, curtime->tm_mday);
			m_NrmFileName.assign(_filename);
			m_DailyPrintHdr = true;
			break;
		}
	default:
		{
			/* do nothing... */
			break;
		}
	}
}

void Logger::CloseLogger()
{
	_CloseFile();
}

void Logger::LogMessage(const char *vafmt, ...)
{
	if (!m_Active)
	{
		return;
	}

	if (m_Mode == LoggingMode_Game)
	{
		va_list ap;
		va_start(ap, vafmt);
		_PrintToGameLog(vafmt, ap);
		va_end(ap);
		return;
	}

	if (m_DelayedStart)
	{
		m_DelayedStart = false;
		_NewMapFile();
	}

	char msg[3072];
	va_list ap;
	va_start(ap, vafmt);
	vsnprintf(msg, sizeof(msg), vafmt, ap);
	va_end(ap);

	char date[32];
	time_t t;
	time(&t);
	tm *curtime = localtime(&t);
	strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);

	FILE *fp = NULL;
	if (m_Mode == LoggingMode_PerMap)
	{
		fp = fopen(m_NrmFileName.c_str(), "a+");
		if (!fp)
		{
			_NewMapFile();
			fp = fopen(m_NrmFileName.c_str(), "a+");
			if (!fp)
			{
				goto print_error;
			}
		}
	} else {
		if (m_CurDay != curtime->tm_mday)
		{
			char _filename[256];
			g_SourceMod.BuildPath(Path_SM, _filename, sizeof(_filename), "logs/logs_%02d%02d.log", curtime->tm_mon + 1, curtime->tm_mday);
			m_NrmFileName.assign(_filename);
			m_CurDay = curtime->tm_mday;
			m_DailyPrintHdr = true;
		}
		fp = fopen(m_NrmFileName.c_str(), "a+");
	}

	if (fp)
	{
		if (m_DailyPrintHdr)
		{
			m_DailyPrintHdr = false;
			fprintf(fp, "L %s: SourceMod log file session started (file \"logs_%02d%02d.log\") (Version \"%s\")\n", date, curtime->tm_mon + 1, curtime->tm_mday, SVN_FULL_VERSION);
	 	}
		fprintf(fp, "L %s: %s\n", date, msg);
		fclose(fp);
	} else {
		goto print_error;
	}

	g_SMAPI->ConPrintf("L %s: %s\n", date, msg);
	return;
print_error:
	g_SMAPI->ConPrint("[SM] Unexpected fatal logging error. SourceMod logging disabled.\n");
	m_Active = false;
}

void Logger::LogError(const char *vafmt, ...)
{
	if (!m_Active)
	{
		return;
	}

	time_t t;
	time(&t);
	tm *curtime = localtime(&t);

	char date[32];
	strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);

	if (curtime->tm_mday != m_CurDay)
	{
		char _filename[256];
		g_SourceMod.BuildPath(Path_SM, _filename, sizeof(_filename), "logs/errors_%02d%02d%02d.log", curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_year - 100);
		m_ErrFileName.assign(_filename);
		m_CurDay = curtime->tm_mday;
		m_ErrMapStart = false;
	}

	char msg[3072];
	va_list ap;
	va_start(ap, vafmt);
	vsnprintf(msg, sizeof(msg), vafmt, ap);
	va_end(ap);

	FILE *fp = fopen(m_ErrFileName.c_str(), "a+");
	if (fp)
	{
		if (!m_ErrMapStart)
		{
			fprintf(fp, "L %s: SourceMod error session started\n", date);
			fprintf(fp, "L %s: Info (map \"%s\") (log file \"errors_%02d%02d%02d.log\")\n", date, m_CurMapName.c_str(), curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_year - 100);
			m_ErrMapStart = true;
		}
		fprintf(fp, "L %s: %s\n", date, msg);
		fclose(fp);
	} else {
		g_SMAPI->ConPrint("[SM] Unexpected fatal logging error. SourceMod logging disabled.\n");
		m_Active = false;
		return;
	}

	g_SMAPI->ConPrintf("L %s: %s\n", date, msg);
}

void Logger::MapChange(const char *mapname)
{
	m_CurMapName.assign(mapname);

	switch (m_Mode)
	{
	case LoggingMode_Daily:
		{
			LogMessage("-------- Mapchange to %s --------", mapname);
			break;
		}
	case LoggingMode_PerMap:
		{
			_NewMapFile();
			break;
		}
	default:
		{
			/* Do nothing... */
			break;
		}
	}

	if (m_ErrMapStart)
	{
		LogError("Error log file session closed.");
	}
	m_ErrMapStart = false;
}

void Logger::_PrintToGameLog(const char *fmt, va_list ap)
{
	char msg[3072];
	size_t len;

	len = vsnprintf(msg, sizeof(msg)-2, fmt, ap);
	len = (len >= sizeof(msg)) ? (sizeof(msg) - 2) : len;

	msg[len++] = '\n';
	msg[len] = '\0';

	engine->LogPrint(msg);
}

const char *Logger::GetLogFileName(LogType type) const
{
	switch (type)
	{
	case LogType_Normal:
		{
			return m_NrmFileName.c_str();
		}
	case LogType_Error:
		{
			return m_ErrFileName.c_str();
		}
	default:
		{
			return "";
		}
	}
}

LoggingMode Logger::GetLoggingMode() const
{
	return m_Mode;
}

void Logger::EnableLogging()
{
	if (m_Active)
	{
		return;
	}
	m_Active = true;
	LogMessage("[SM] Logging enabled manually by user.");
}

void Logger::DisableLogging()
{
	if (!m_Active)
	{
		return;
	}
	LogMessage("[SM] Logging disabled manually by user.");
	m_Active = false;
}

void Logger::LogFatal(const char *msg, ...)
{
	/* :TODO: make this print all pretty-like
	 * In fact, the pretty log printing function should be abstracted. 
	 * It's already implemented twice which is bad.
	 */

	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "sourcemod_fatal.log");
	FILE *fp = fopen(path, "at");
	if (!fp)
	{
		/* We're just doomed, aren't we... */
		return;
	}

	va_list ap;
	va_start(ap, msg);
	vfprintf(fp, msg, ap);
	va_end(ap);

	fputs("\n", fp);
	
	fclose(fp);
}
