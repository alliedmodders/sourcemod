#ifndef _INCLUDE_SOURCEMOD_CLOGGER_H_
#define _INCLUDE_SOURCEMOD_CLOGGER_H_

#include <time.h>
#include "sourcemm_api.h"
#include <sh_string.h>
#include "sourcemod.h"
#include "sm_version.h"

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

class CLogger
{
public:
	CLogger() : m_ErrMapStart(false), m_Active(false), m_DelayedStart(false), m_DailyPrintHdr(false) {}
public:
	void InitLogger(LoggingMode mode, bool startlogging);
	void CloseLogger();
	void EnableLogging();
	void DisableLogging();
	void LogMessage(const char *msg, ...);
	void LogMessageEx(LogType type, const char *msg, ...);
	void MapChange(const char *mapname);
	const char *GetLogFileName(LogType type);
	LoggingMode GetLoggingMode();
private:
	void _CloseFile();
	void _NewMapFile();
	void _PrintToHL2Log(const char *fmt, ...);
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

extern CLogger g_Logger;

inline const char *CLogger::GetLogFileName(LogType type)
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

inline LoggingMode CLogger::GetLoggingMode()
{
	return m_mode;
}

inline void CLogger::EnableLogging()
{
	if (m_Active)
	{
		return;
	}
	m_Active = true;
	LogMessage("Logging enabled manually by user.");
}

inline void CLogger::DisableLogging()
{
	if (!m_Active)
	{
		return;
	}
	LogMessage("Logging disabled manually by user.");
	m_Active = false;
}

#endif // _INCLUDE_SOURCEMOD_CLOGGER_H_