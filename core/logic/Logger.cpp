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
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be \"on\" or \"off\"");
			return ConfigResult_Reject;
		}

		if (source == ConfigSource_Console)
		{
			state ? EnableLogging() : DisableLogging();
		} else {
			m_Active = state;
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
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be [daily|map|game]");
			return ConfigResult_Reject;
		}

		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void Logger::OnSourceModStartup(bool late)
{
	char buff[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, buff, sizeof(buff), "logs");
	if (!libsys->IsPathDirectory(buff))
	{
		libsys->CreateFolder(buff);
	}
}

void Logger::OnSourceModAllShutdown()
{
	CloseLogger();
}

void Logger::OnSourceModLevelChange(const char *mapName)
{
	_MapChange(mapName);
}

void Logger::CloseLogger()
{
	_CloseFile();
}

void Logger::_CloseFile()
{
	_CloseNormal();
	_CloseError();
	_CloseFatal();
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

	fflush(fp);
}

void Logger::LogToFileOnlyEx(FILE *fp, const char *msg, va_list ap)
{
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

	FILE *pFile = _OpenNormal();
	if (!pFile)
	{
		return;
	}

	LogToOpenFileEx(pFile, vafmt, ap);
	fclose(pFile);
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

	FILE *pFile = _OpenError();
	if (!pFile)
	{
		return;
	}

	LogToOpenFileEx(pFile, vafmt, ap);
	fclose(pFile);
}

void Logger::_MapChange(const char *mapname)
{
	m_CurrentMapName = mapname;
	_UpdateFiles(true);
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

	FILE *pFile = _OpenFatal();
	if (!pFile)
	{
		return;
	}

	LogToOpenFileEx(pFile, msg, ap);
	fclose(pFile);
}

void Logger::_UpdateFiles(bool bLevelChange)
{
	time_t t = g_pSM->GetAdjustedTime();
	tm *curtime = localtime(&t);

	if (!bLevelChange && curtime->tm_mday == m_Day)
	{
		return;
	}

	m_Day = curtime->tm_mday;

	char buff[PLATFORM_MAX_PATH];
	ke::SafeSprintf(buff, sizeof(buff), "%04d%02d%02d", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday);

	std::string currentDate(buff);

	if (m_Mode == LoggingMode_PerMap)
	{
		if (bLevelChange)
		{
			for (size_t iter = 0; iter < static_cast<size_t>(-1); ++iter)
			{
				g_pSM->BuildPath(Path_SM, buff, sizeof(buff), "logs/L%s%u.log", currentDate.c_str(), iter);
				if (!libsys->IsPathFile(buff))
				{
					break;
				}
			}
		}
		else
		{
			ke::SafeStrcpy(buff, sizeof(buff), m_NormalFileName.c_str());
		}
	}
	else
	{
		g_pSM->BuildPath(Path_SM, buff, sizeof(buff), "logs/L%s.log", currentDate.c_str());
	}

	if (m_NormalFileName.compare(buff))
	{
		_CloseNormal();
		m_NormalFileName = buff;
	}
	else
	{
		if (bLevelChange)
		{
			LogMessage("-------- Mapchange to %s --------", m_CurrentMapName.c_str());
		}
	}

	g_pSM->BuildPath(Path_SM, buff, sizeof(buff), "logs/errors_%s.log", currentDate.c_str());
	if (bLevelChange || m_ErrorFileName.compare(buff))
	{
		_CloseError();
		m_ErrorFileName = buff;
	}
}

FILE *Logger::_OpenNormal()
{
	_UpdateFiles();

	FILE *pFile = fopen(m_NormalFileName.c_str(), "a+");
	if (pFile == NULL)
	{
		_LogFatalOpen(m_NormalFileName);
		return pFile;
	}

	if (!m_DamagedNormalFile)
	{
		time_t t = g_pSM->GetAdjustedTime();
		tm *curtime = localtime(&t);
		char date[32];

		strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
		fprintf(pFile, "L %s: SourceMod log file session started (file \"%s\") (Version \"%s\")\n", date, m_NormalFileName.c_str(), SOURCEMOD_VERSION);
		m_DamagedNormalFile = true;
	}

	return pFile;
}

FILE *Logger::_OpenError()
{
	_UpdateFiles();

	FILE *pFile = fopen(m_ErrorFileName.c_str(), "a+");
	if (pFile == NULL)
	{
		_LogFatalOpen(m_ErrorFileName);
		return pFile;
	}

	if (!m_DamagedErrorFile)
	{
		time_t t = g_pSM->GetAdjustedTime();
		tm *curtime = localtime(&t);

		char date[32];
		strftime(date, sizeof(date), "%m/%d/%Y - %H:%M:%S", curtime);
		fprintf(pFile, "L %s: SourceMod error session started\n", date);
		fprintf(pFile, "L %s: Info (map \"%s\") (file \"%s\")\n", date, m_CurrentMapName.c_str(), m_ErrorFileName.c_str());
		m_DamagedErrorFile = true;
	}

	return pFile;
}

FILE *Logger::_OpenFatal()
{
	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "sourcemod_fatal.log");
	return fopen(path, "at");
}

void Logger::_LogFatalOpen(std::string &str)
{
	char error[255];
	libsys->GetPlatformError(error, sizeof(error));
	LogFatal("[SM] Unexpected fatal logging error (file \"%s\")", str.c_str());
	LogFatal("[SM] Platform returned error: \"%s\"", error);
}

void Logger::_CloseNormal()
{
	if (m_DamagedNormalFile)
	{
		LogMessage("Log file closed.");
		m_DamagedNormalFile = false;
	}
}

void Logger::_CloseError()
{
	if (m_DamagedErrorFile)
	{
		LogError("Error log file session closed.");
		m_DamagedErrorFile = false;
	}
}

void Logger::_CloseFatal()
{
}
