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

#ifndef _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
#define _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_

#include <IGameConfigs.h>
#include <ITextParsers.h>
//#include <sh_list.h>
#include "sm_trie.h"
//#include "sm_globals.h"
#include "sm_memtable.h"
#include "sm_trie_tpl.h"
//#include "ThreadSupport.h"
#include <list>
#include <map>
#include "gdc.h"
#include <limits.h>
#include <string>
#include <cstring>
#include <cstdio>

#define PLATFORM_MAX_PATH PATH_MAX

using std::map;
using std::list;
using namespace SourceMod;
//using namespace SourceHook;

extern char *game;
extern char *engine;

#define PARSE_ERROR -1
struct cmp_str 
{
	bool operator()(char const *a, char const *b) 
	{
		return std::strcmp(a, b) < 0;
	}
};

struct Offset
{
	char *name;
	char *symbol;
	int lin;
	int win;

	Offset()
	{
		name = symbol = NULL;
		lin = win = -1;
	}

	~Offset()
	{
		delete name;
		delete symbol;
	}

	void reset()
	{
		delete name;
		delete symbol;
		lin = win = -1;
	}

	void setName(const char *src)
	{
		delete name;
		name = new char[strlen(src)+1];
		strcpy(name, src);
	}

	void setSymbol(const char *src)
	{
		delete symbol;
		symbol = new char[strlen(src)+1];
		strcpy(symbol, src);
	}

	void setLin(int offs)
	{
		lin = offs;
	}

	void setWin(int offs)
	{
		win = offs;
	}
};

enum Library
{
	Engine,
	Server
};

struct Sig
{
	char *name;
	char *win;
	char *lin;
	Library lib;

	Sig()
	{
		name = win = lin = NULL;
		lib = (Library) -1;
	}

	~Sig()
	{
		delete name;
		delete win;
		delete lin;
	}

	void reset()
	{
		delete name;
		delete win;
		delete lin;
		lib = (Library) -1;
	}

	void setName(const char *src)
	{
		delete name;
		name = new char[strlen(src)+1];
		strcpy(name, src);
	}

	void setWin(const char *src)
	{
		delete win;
		win = new char[strlen(src)+1];
		strcpy(win, src);
	}

	void setLin(const char *src)
	{
		delete lin;
		lin = new char[strlen(src)+1];
		strcpy(lin, src);
	}

	void setLib(const char *src)
	{
		if (stricmp(src, "server") == 0) lib = Server;
		else if (stricmp(src, "engine") == 0) lib = Engine;
		else lib = (Library)-1;
	}
};

class SendProp;

class CGameConfig : 
	public ITextListener_SMC//,
//	public IGameConfig
{
public:
	CGameConfig();//const char *file);//, const char *game, const char *engine);
	~CGameConfig();
public:
	bool Reparse(char *error, size_t maxlength);
	bool EnterFile(const char *file, char *error, size_t maxlength);
	list<Offset> GetOffsets();
	list<Sig> GetSigs();
public: //ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public: //IGameConfig
	const char *GetKeyValue(const char *key);
	const char *GetOptionValue(const char *key);
	bool GetOffset(const char *key, int *value);
	SendProp *GetSendProp(const char *key);
	bool GetMemSig(const char *key, void **addr);
public:
//	void IncRefCount();
//	unsigned int DecRefCount();
private:
//	bool DoesGameMatch(const char *value);
//	bool DoesEngineMatch(const char *value);
public:
	BaseStringTable *m_pStrings;
	char m_File[PLATFORM_MAX_PATH];
	char m_CurFile[PLATFORM_MAX_PATH];
	list<Offset> m_Offsets;
	list<Sig> m_Sigs;
//	map<const char*,const char*,cmp_str> m_Keys;
	KTrie<int> m_Keys;
	KTrie<int> m_Options;
	unsigned int m_RefCount;
	/* Parse states */
	int m_ParseState;
	unsigned int m_IgnoreLevel;
	char m_Class[64];
	char m_Prop[64];
	char m_offset[64];
	char m_Game[256];
	char m_gdcGame[256];
	char m_gdcEngine[256];
	bool bShouldBeReadingDefault;
	bool had_game;
	bool matched_game;
	bool had_engine;
	bool matched_engine;

	/* Custom Sections */
	unsigned int m_CustomLevel;
	ITextListener_SMC *m_CustomHandler;
};

#endif //_INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
