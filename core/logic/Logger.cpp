/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#include <time.h>
#include <cstdarg>
#include "Logger.h"
#include <sourcemod_version.h>
#include <ISourceMod.h>
#include <am-string.h>
#include <ILibrarySys.h>
#include <bridge/include/CoreProvider.h>

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
			ke::SafeSprintf(error, maxlength, "Invalid value: must be \"on\" or \"off\"");
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
			ke::SafeSprintf(error, maxlength, "Invalid value: must be [daily|map|game]");
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

void Logger::OnSourceModLevelChange(const char *mapName)
{
	MapChange(mapName);
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

	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);

	while (true)
	{
		g_pSM->BuildPath(Path_SM, _filename, sizeof(_filename), "logs/L%02d%02d%03d.log", curtime->tm_mon + 1, curtime->tm_mday, i);
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
		char error[255];
		libsys->GetPlatformError(error, sizeof(error));
		LogFatal("[SM] Unexpected fatal logging error (file \"%s\")", m_NrmFileName.c_str());
		LogFatal("[SM] Platform returned error: \"%s\"", error);
		LogFatal("[SM] Logging has been disabled.");
		m_Active = false;
		return;
	} else {
		char date[32];
		strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
		fprintf(fp, "L %s: SourceMod log file started (file \"L%02d%02d%03d.log\") (Version \"%s\")\n", date, curtime->tm_mon + 1, curtime->tm_mday, i, SOURCEMOD_VERSION);
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

	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);
	m_NrmCurDay = curtime->tm_mday;
	m_ErrCurDay = curtime->tm_mday;

	char _filename[256];
	g_pSM->BuildPath(Path_SM, _filename, sizeof(_filename), "logs/errors_%04d%02d%02d.log", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);
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
			g_pSM->BuildPath(Path_SM, _filename, sizeof(_filename), "logs/L%04d%02d%02d.log", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);
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

void Logger::LogToOpenFile(FILE *fp, const char *msg, ...)
{
	if (!m_Active)
	{
		return;
	}

	va_list ap;
	va_start(ap, msg);
	LogToOpenFileEx(fp, msg, ap);
	va_end(ap);
}

void Logger::LogToFileOnly(FILE *fp, const char *msg, ...)
{
	if (!m_Active)
	{
		return;
	}

	va_list ap;
	va_start(ap, msg);
	LogToFileOnlyEx(fp, msg, ap);
	va_end(ap);
}

void Logger::LogToOpenFileEx(FILE *fp, const char *msg, va_list ap)
{
	if (!m_Active)
	{
		return;
	}

	static ConVar *sv_logecho = bridge->FindConVar("sv_logecho");

	char buffer[3072];
	ke::SafeVsprintf(buffer, sizeof(buffer), msg, ap);

	char date[32];
	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);
	strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);

	fprintf(fp, "L %s: %s\n", date, buffer);

	if (!sv_logecho || bridge->GetCvarBool(sv_logecho))
	{
		static char conBuffer[4096];
		ke::SafeSprintf(conBuffer, sizeof(conBuffer), "L %s: %s\n", date, buffer);
		bridge->ConPrint(conBuffer);
	}
}

void Logger::LogToFileOnlyEx(FILE *fp, const char *msg, va_list ap)
{
	if (!m_Active)
	{
		return;
	}

	char buffer[3072];
	ke::SafeVsprintf(buffer, sizeof(buffer), msg, ap);

	char date[32];
	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);
	strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);

	fprintf(fp, "L %s: %s\n", date, buffer);
	fflush(fp);
}

void Logger::LogMessage(const char *vafmt, ...)
{
	va_list ap;
	va_start(ap, vafmt);
	LogMessageEx(vafmt, ap);
	va_end(ap);
}

void Logger::LogMessageEx(const char *vafmt, va_list ap)
{
	if (!m_Active)
	{
		return;
	}

	if (m_Mode == LoggingMode_Game)
	{
		_PrintToGameLog(vafmt, ap);
		return;
	}

	if (m_DelayedStart)
	{
		m_DelayedStart = false;
		_NewMapFile();
	}

	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);

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
		if (m_NrmCurDay != curtime->tm_mday)
		{
			char _filename[256];
			g_pSM->BuildPath(Path_SM, _filename, sizeof(_filename), "logs/L%04d%02d%02d.log", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);
			m_NrmFileName.assign(_filename);
			m_NrmCurDay = curtime->tm_mday;
			m_DailyPrintHdr = true;
		}
		fp = fopen(m_NrmFileName.c_str(), "a+");
	}

	if (fp)
	{
		if (m_DailyPrintHdr)
		{
			char date[32];
			m_DailyPrintHdr = false;
			strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
			fprintf(fp, "L %s: SourceMod log file session started (file \"L%04d%02d%02d.log\") (Version \"%s\")\n", date, curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday, SOURCEMOD_VERSION);
	 	}
		LogToOpenFileEx(fp, vafmt, ap);
		fclose(fp);
	} else {
		goto print_error;
	}

	return;
print_error:
	char error[255];
	libsys->GetPlatformError(error, sizeof(error));
	LogFatal("[SM] Unexpected fatal logging error (file \"%s\")", m_NrmFileName.c_str());
	LogFatal("[SM] Platform returned error: \"%s\"", error);
	LogFatal("[SM] Logging has been disabled.");
	m_Active = false;
}

void Logger::LogError(const char *vafmt, ...)
{
	va_list ap;
	va_start(ap, vafmt);
	LogErrorEx(vafmt, ap);
	va_end(ap);
}

void Logger::LogErrorEx(const char *vafmt, va_list ap)
{
	if (!m_Active)
	{
		return;
	}

	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);

	if (curtime->tm_mday != m_ErrCurDay)
	{
		char _filename[256];
		g_pSM->BuildPath(Path_SM, _filename, sizeof(_filename), "logs/errors_%04d%02d%02d.log", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);
		m_ErrFileName.assign(_filename);
		m_ErrCurDay = curtime->tm_mday;
		m_ErrMapStart = false;
	}

	FILE *fp = fopen(m_ErrFileName.c_str(), "a+");
	if (fp)
	{
		if (!m_ErrMapStart)
		{
			char date[32];
			strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
			fprintf(fp, "L %s: SourceMod error session started\n", date);
			fprintf(fp, "L %s: Info (map \"%s\") (file \"errors_%04d%02d%02d.log\")\n", date, m_CurMapName.c_str(), curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);
			m_ErrMapStart = true;
		}
		LogToOpenFileEx(fp, vafmt, ap);
		fclose(fp);
	}
	else
	{
		char error[255];
		libsys->GetPlatformError(error, sizeof(error));
		LogFatal("[SM] Unexpected fatal logging error (file \"%s\")", m_NrmFileName.c_str());
		LogFatal("[SM] Platform returned error: \"%s\"", error);
		LogFatal("[SM] Logging has been disabled.");
		m_Active = false;
		return;
	}
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

	bridge->LogToGame(msg);
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
	va_list ap;
	va_start(ap, msg);
	LogFatalEx(msg, ap);
	va_end(ap);
}

void Logger::LogFatalEx(const char *msg, va_list ap)
{
	/* :TODO: make this print all pretty-like
	 * In fact, the pretty log printing function should be abstracted. 
	 * It's already implemented twice which is bad.
	 */

	char path[PLATFORM_MAX_PATH];

	g_pSM->BuildPath(Path_Game, path, sizeof(path), "sourcemod_fatal.log");

	FILE *fp = fopen(path, "at");
	if (fp)
	{
		m_Active = true;
		LogToOpenFileEx(fp, msg, ap);
		m_Active = false;
		fclose(fp);
	}
}
