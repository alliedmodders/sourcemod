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

#ifndef _INCLUDE_SOURCEMOD_GAMEDATAFETCHER_H_
#define _INCLUDE_SOURCEMOD_GAMEDATAFETCHER_H_

#include "sourcemod.h"
#include "TextParsers.h"
#include "IThreader.h"
#include "Logger.h"
#include "LibrarySys.h"
#include "ThreadSupport.h"
#include "sm_memtable.h"
#include <sh_string.h>
#include <sh_list.h>

enum UpdateStatus
{ 
	Update_Unknown = 0,				/* Version wasn't recognised or version querying is unsupported */
	Update_Current = 1,				/* Server is running latest version */
	Update_NewBuild = 2,			/* Server is on a svn release and a newer version is available */
	Update_MinorAvailable = 3,		/* Server is on a release and a minor release has superceeded it */
	Update_MajorAvailable = 4,		/* Server is on a release and a major release has superceeded it */
	Update_CriticalAvailable = 5,	/* A critical update has been released (security fixes etc) */
};

class MD5;

class BuildMD5ableBuffer : public ITextListener_SMC
{
public:
	BuildMD5ableBuffer();
	~BuildMD5ableBuffer();
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	void ReadSMC_ParseEnd(bool halted, bool failed);
public:
	MD5 *checksum;
private:
	BaseStringTable *stringTable;
};

struct FileData
{
	SourceHook::String *filename;
	char checksum[33];
};

class FetcherThread : public IThread
{
public:
	FetcherThread();
	~FetcherThread();
public:
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);
private:
	int BuildGameDataQuery(char *buffer, int maxlen);
	void ProcessGameDataQuery(int SocketDescriptor);
	int RecvData(int socketDescriptor, char *buffer, int len);
	int SendData(int socketDescriptor, char *buffer, int len);
	int ConnectSocket();
	void HandleUpdateStatus(UpdateStatus status, short version[4]);
public:
	SourceHook::CVector<FileData *> filenames;
private:
	bool wasSuccess;
	bool needsRestart;
	UpdateStatus updateStatus;
	BaseMemTable *memtable;
	short build[4];
};

extern BuildMD5ableBuffer g_MD5Builder;
extern bool g_blockGameDataLoad;

#endif // _INCLUDE_SOURCEMOD_GAMEDATAFETCHER_H_

