/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#include <sh_list.h>
#include <sm_namehashset.h>
#include "common_logic.h"
#include "CellArray.h"
#include <IGameHelpers.h>
#include <ILibrarySys.h>
#include <ITextParsers.h>
#include <ISourceMod.h>
#include "stringutil.h"
#include <bridge/include/CoreProvider.h>
#include <bridge/include/ILogger.h>
#include <bridge/include/IFileSystemBridge.h>

using namespace SourceHook;

struct maplist_info_t
{
	bool bIsCompat;
	bool bIsPath;
	char name[PLATFORM_MAX_PATH];
	char path[PLATFORM_MAX_PATH];
	time_t last_modified_time;
	CellArray *pArray;
	int serial;

	static inline bool matches(const char *key, const maplist_info_t *value)
	{
		return strcmp(value->name, key) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

#define MAPLIST_FLAG_MAPSFOLDER		(1<<0)		/**< On failure, use all maps in the maps folder. */
#define MAPLIST_FLAG_CLEARARRAY		(1<<1)		/**< If an input array is specified, clear it before adding. */
#define MAPLIST_FLAG_NO_DEFAULT		(1<<2)		/**< Do not read "default" or "mapcyclefile" on failure. */

class MapLists : public SMGlobalClass, public ITextListener_SMC
{
public:
	enum MapListState
	{
		MPS_NONE,
		MPS_GLOBAL,
		MPS_MAPLIST,
	};
public:
	MapLists()
	{
		m_pMapCycleFile = NULL;
		m_ConfigLastChanged = 0;
		m_nSerialChange = 0;
	}
	void OnSourceModAllInitialized()
	{
		g_pSM->BuildPath(Path_SM, m_ConfigFile, sizeof(m_ConfigFile), "configs/maplists.cfg");
	}
	void OnSourceModShutdown()
	{
		DumpCache(NULL);
	}
	void GetMapCycleFilePath(char *pBuffer, int maxlen)
	{
		const char *pMapCycleFileName = m_pMapCycleFile ? bridge->GetCvarString(m_pMapCycleFile) : "mapcycle.txt";

		g_pSM->Format(pBuffer, maxlen, "cfg/%s", pMapCycleFileName);
		if (!bridge->filesystem->FileExists(pBuffer, "GAME"))
		{
			g_pSM->Format(pBuffer, maxlen, "%s", pMapCycleFileName);
			if (!bridge->filesystem->FileExists(pBuffer, "GAME"))
			{
				g_pSM->Format(pBuffer, maxlen, "cfg/mapcycle_default.txt");
			}
		}
	}
	void AddOrUpdateDefault(const char *name, const char *file)
	{
		char path[PLATFORM_MAX_PATH];
		maplist_info_t *pMapList;

		if (!m_ListLookup.retrieve(name, &pMapList))
		{
			pMapList = new maplist_info_t;
			pMapList->bIsCompat = true;
			pMapList->bIsPath = true;
			pMapList->last_modified_time = 0;
			strncopy(pMapList->name, name, sizeof(pMapList->name));
			pMapList->pArray = NULL;
			strncopy(pMapList->path, file, sizeof(pMapList->path));
			pMapList->serial = 0;
			m_ListLookup.insert(name, pMapList);
			m_MapLists.push_back(pMapList);
			return;
		}

		/* Don't modify if it's from the config file */
		if (!pMapList->bIsCompat)
			return;

		strncopy(path, file, sizeof(path));

		/* If the path matches, don't reset the serial/time */
		if (strcmp(path, pMapList->path) == 0)
			return;

		strncopy(pMapList->path, path, sizeof(pMapList->path));
		pMapList->bIsPath = true;
		pMapList->last_modified_time = 0;
		pMapList->serial = 0;
	}
	void UpdateCache()
	{
		bool fileFound;
		SMCError error;
		time_t fileTime;
		SMCStates states = {0, 0};
		
		fileFound = libsys->FileTime(m_ConfigFile, FileTime_LastChange, &fileTime);

		/* If the file is found and hasn't changed, bail out now. */
		if (fileFound && fileTime == m_ConfigLastChanged)
		{
			return;
		}

		/* If the file wasn't found, and we already have entries, we bail out too. 
		 * This case lets us optimize when a user deletes the config file, so we 
		 * don't reparse every single time the function is called.
		 */
		if (!fileFound && m_MapLists.size() > 0)
		{
			return;
		}

		m_pMapCycleFile = bridge->FindConVar("mapcyclefile");

		/* Dump everything we know about. */
		List<maplist_info_t *> compat;
		DumpCache(&compat);

		/* All this is to add the default entry back in. */
		maplist_info_t *pDefList = new maplist_info_t;

		pDefList->bIsPath = true;
		strncopy(pDefList->name, "mapcyclefile", sizeof(pDefList->name));

		GetMapCycleFilePath(pDefList->path, sizeof(pDefList->path));
		
		pDefList->last_modified_time = 0;
		pDefList->pArray = NULL;
		pDefList->serial = 0;

		m_ListLookup.insert("mapcyclefile", pDefList);
		m_MapLists.push_back(pDefList);

		/* Now parse the config file even if we don't know about it.
		 * This will give us a nice error message.
		 */
		if ((error = textparser->ParseFile_SMC(m_ConfigFile, this, &states))
			!= SMCError_Okay)
		{
			const char *errmsg = textparser->GetSMCErrorString(error);
			if (errmsg == NULL)
			{
				errmsg = "Unknown error";
			}
			logger->LogError("[SM] Could not parse file \"%s\"", m_ConfigFile);
			logger->LogError("[SM] Error on line %d (col %d): %s", states.line, states.col, errmsg);
		}
		else
		{
			m_ConfigLastChanged = fileTime;
		}

		/* Now, re-add compat stuff back in if we can. */
		List<maplist_info_t *>::iterator iter = compat.begin();
		while (iter != compat.end())
		{
			if (m_ListLookup.contains((*iter)->name))
			{
				/* The compatibility shim is no longer needed. */
				delete (*iter)->pArray;
				delete (*iter);
			}
			else
			{
				m_ListLookup.insert((*iter)->name, (*iter));
				m_MapLists.push_back((*iter));
			}
			iter = compat.erase(iter);
		}
	}
	void ReadSMC_ParseStart()
	{
		m_CurState = MPS_NONE;
		m_IgnoreLevel = 0;
		m_pCurMapList = NULL;
	}
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		if (m_IgnoreLevel)
		{
			m_IgnoreLevel++;
			return SMCResult_Continue;
		}

		if (m_CurState == MPS_NONE)
		{
			if (strcmp(name, "MapLists") == 0)
			{
				m_CurState = MPS_GLOBAL;
			}
			else
			{
				m_IgnoreLevel = 1;
			}
		}
		else if (m_CurState == MPS_GLOBAL)
		{
			m_pCurMapList = new maplist_info_t;

			memset(m_pCurMapList, 0, sizeof(maplist_info_t));
			strncopy(m_pCurMapList->name, name, sizeof(m_pCurMapList->name));

			m_CurState = MPS_MAPLIST;
		}
		else if (m_CurState == MPS_MAPLIST)
		{
			m_IgnoreLevel++;
		}

		return SMCResult_Continue;
	}
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		if (m_IgnoreLevel || m_pCurMapList == NULL)
		{
			return SMCResult_Continue;
		}

		if (strcmp(key, "file") == 0)
		{
			strncopy(m_pCurMapList->path, value, sizeof(m_pCurMapList->path));
			m_pCurMapList->bIsPath = true;
		}
		else if (strcmp(key, "target") == 0)
		{
			strncopy(m_pCurMapList->path, value, sizeof(m_pCurMapList->path));
			m_pCurMapList->bIsPath = false;
		}

		return SMCResult_Continue;
	}
	SMCResult ReadSMC_LeavingSection(const SMCStates *states)
	{
		if (m_IgnoreLevel)
		{
			m_IgnoreLevel--;
			return SMCResult_Continue;
		}

		if (m_CurState == MPS_MAPLIST)
		{
			if (m_pCurMapList != NULL
				&& m_pCurMapList->path[0] != '\0'
				&& !m_ListLookup.contains(m_pCurMapList->name))
			{
				m_ListLookup.insert(m_pCurMapList->name, m_pCurMapList);
				m_MapLists.push_back(m_pCurMapList);
				m_pCurMapList = NULL;
			}
			else
			{
				delete m_pCurMapList;
				m_pCurMapList = NULL;
			}
			m_CurState = MPS_GLOBAL;
		}
		else if (m_CurState == MPS_GLOBAL)
		{
			m_CurState = MPS_NONE;
		}

		return SMCResult_Continue;
	}
	void ReadSMC_ParseEnd(bool halted, bool failed)
	{
		delete m_pCurMapList;
		m_pCurMapList = NULL;
	}
	static bool alphanum_isdigit(const char c)
	{
		return c >= '0' && c <= '9';
	}
	static int alphanum_impl(const char *l, const char *r)
	{
		/**
		 * http://www.davekoelle.com/files/alphanum.hpp
		 *
		 * compare l and r with strcmp() semantics, but using
		 * the "Alphanum Algorithm". This function is designed to read
		 * through the l and r strings only one time, for
		 * maximum performance. It does not allocate memory for
		 * substrings.
		 *
		 * Released under the MIT License - https://opensource.org/licenses/MIT
		 *
		 * Permission is hereby granted, free of charge, to any person obtaining
		 * a copy of this software and associated documentation files (the "Software"),
		 * to deal in the Software without restriction, including without limitation
		 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
		 * and/or sell copies of the Software, and to permit persons to whom the
		 * Software is furnished to do so, subject to the following conditions:
		 *
		 * The above copyright notice and this permission notice shall be included
		 * in all copies or substantial portions of the Software.
		 *
		 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
		 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
		 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
		 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
		 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
		 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
		 * USE OR OTHER DEALINGS IN THE SOFTWARE.
		 */

		enum mode_t { STRING, NUMBER } mode = STRING;

		while (*l && *r)
		{
			if (mode == STRING)
			{
				char l_char, r_char;
				while ((l_char = *l) && (r_char = *r))
				{
					// check if this are digit characters
					const bool l_digit = alphanum_isdigit(l_char), r_digit = alphanum_isdigit(r_char);
					// if both characters are digits, we continue in NUMBER mode
					if (l_digit && r_digit)
					{
						mode = NUMBER;
						break;
					}
					// if only the left character is a digit, we have a result
					if (l_digit) return -1;
					// if only the right character is a digit, we have a result
					if (r_digit) return +1;
					// compute the difference of both characters
					const int diff = l_char - r_char;
					// if they differ we have a result
					if (diff != 0) return diff;
					// otherwise process the next characters
					++l;
					++r;
				}
			}
			else // mode == NUMBER
			{
				// get the left number
				unsigned long l_int = 0;
				while (*l && alphanum_isdigit(*l))
				{
					// TODO: this can overflow
					l_int = l_int * 10 + *l - '0';
					++l;
				}

				// get the right number
				unsigned long r_int = 0;
				while (*r && alphanum_isdigit(*r))
				{
					// TODO: this can overflow
					r_int = r_int * 10 + *r - '0';
					++r;
				}

				// if the difference is not equal to zero, we have a comparison result
				const long diff = l_int - r_int;
				if (diff != 0)
					return diff;

				// otherwise we process the next substring in STRING mode
				mode = STRING;
			}
		}

		if (*r) return -1;
		if (*l) return +1;
		return 0;
	}
	static int sort_maps_in_adt_array(const void *str1, const void *str2)
	{
		return alphanum_impl(static_cast<const char *>(str1), static_cast<const char *>(str2));
	}
	ICellArray *UpdateMapList(ICellArray *pUseArray, const char *name, int *pSerial, unsigned int flags)
	{
		int change_serial;
		CellArray *pNewArray = NULL;
		bool success, free_new_array;

		free_new_array = false;
		
		if ((success = GetMapList(&pNewArray, name, &change_serial)) == false)
		{
			if ((flags & MAPLIST_FLAG_NO_DEFAULT) != MAPLIST_FLAG_NO_DEFAULT)
			{
				/* If this list failed, and it's not the default, try the default. 
				 */
				if (strcmp(name, "default") != 0)
				{
					success = GetMapList(&pNewArray, "default", &change_serial);
				}
				/* If either of the last two conditions failed, try again if we can. */
				if (!success && strcmp(name, "mapcyclefile") != 0)
				{
					success = GetMapList(&pNewArray, "mapcyclefile", &change_serial);
				}
			}
		}

		/* If there was a success, and the serial has not changed, bail out. */
		if (success && *pSerial == change_serial)
		{
			return NULL;
		}

		/**
		 * If there was a success but no map list, we need to look in the maps folder.
		 * If there was a failure and the flag is specified, we need to look in the maps folder.
		 */
		if ((success && pNewArray == NULL)
			|| (!success && ((flags & MAPLIST_FLAG_MAPSFOLDER) == MAPLIST_FLAG_MAPSFOLDER)))
		{
			pNewArray = new CellArray(64);
			free_new_array = true;

			cell_t *blk;

			FileFindHandle_t findHandle;
			const char *fileName = bridge->filesystem->FindFirstEx("maps/*.bsp", "GAME", &findHandle);

			while (fileName)
			{
				char buffer[PLATFORM_MAX_PATH];

				UTIL_StripExtension(fileName, buffer, sizeof(buffer));

				if (!gamehelpers->IsMapValid(buffer))
				{
					fileName = bridge->filesystem->FindNext(findHandle);
					continue;
				}

				if ((blk = pNewArray->push()) == NULL)
				{
					fileName = bridge->filesystem->FindNext(findHandle);
					continue;
				}

				strncopy((char *)blk, buffer, 255);

				fileName = bridge->filesystem->FindNext(findHandle);
			}

			bridge->filesystem->FindClose(findHandle);

			/* Remove the array if there were no items. */
			if (pNewArray->size() == 0)
			{
				delete pNewArray;
				pNewArray = NULL;
			}
			else
			{
				qsort(pNewArray->base(), 
					pNewArray->size(), 
					pNewArray->blocksize() * sizeof(cell_t), 
					sort_maps_in_adt_array);
			}

			change_serial = -1;
		}

		/* If there is still no array by this point, bail out. */
		if (pNewArray == NULL)
		{
			*pSerial = -1;
			return NULL;
		}

		*pSerial = change_serial;

		/* If there is no input array, return something temporary. */
		if (pUseArray == NULL)
		{
			if (free_new_array)
			{
				return pNewArray;
			}
			else
			{
				return pNewArray->clone();
			}
		}

		/* Clear the input array if necessary. */
		if ((flags & MAPLIST_FLAG_CLEARARRAY) == MAPLIST_FLAG_CLEARARRAY)
		{
			pUseArray->clear();
		}

		/* Copy. */
		cell_t *blk_dst;
		cell_t *blk_src;
		for (size_t i = 0; i < pNewArray->size(); i++)
		{
			blk_dst = pUseArray->push();
			blk_src = pNewArray->at(i);
			strncopy((char *)blk_dst, (char *)blk_src, pUseArray->blocksize() * sizeof(cell_t));
		}

		/* Free resources if necessary. */
		if (free_new_array)
		{
			delete pNewArray;
		}

		/* Return the array we were given. */
		return pUseArray;
	}
private:
	bool GetMapList(CellArray **ppArray, const char *name, int *pSerial)
	{
		time_t last_time;
		maplist_info_t *pMapList;

		if (!m_ListLookup.retrieve(name, &pMapList))
			return false;

		if (!pMapList->bIsPath)
			return GetMapList(ppArray, pMapList->path, pSerial);

		/* If it is a path, and the path is "*", assume all files must be used. */
		if (strcmp(pMapList->path, "*") == 0)
		{
			*ppArray = NULL;
			return true;
		}

		if (m_pMapCycleFile != NULL && strcmp(name, "mapcyclefile") == 0)
		{
			char path[PLATFORM_MAX_PATH];
			GetMapCycleFilePath(path, sizeof(path));

			if (strcmp(path, pMapList->path) != 0)
			{
				strncopy(pMapList->path, path, sizeof(pMapList->path));
				pMapList->last_modified_time = 0;
			}
		}

		if (!libsys->FileTime(pMapList->path, FileTime_LastChange, &last_time)
			|| last_time > pMapList->last_modified_time)
		{
			/* Reparse */
			FileHandle_t fp;
			cell_t *blk;
			char buffer[255];

			if ((fp = bridge->filesystem->Open(pMapList->path, "rt", "GAME")) == NULL)
			{
				return false;
			}

			delete pMapList->pArray;
			pMapList->pArray = new CellArray(64);

			while (!bridge->filesystem->EndOfFile(fp) && bridge->filesystem->ReadLine(buffer, sizeof(buffer), fp) != NULL)
			{
				size_t len = strlen(buffer);
				char *ptr = UTIL_TrimWhitespace(buffer, len);
				if (*ptr == '\0'
					|| *ptr == ';'
					|| strncmp(ptr, "//", 2) == 0)
				{
					continue;
				}
				
				if (strcmp(bridge->GetSourceEngineName(), "insurgency") == 0
					|| strcmp(bridge->GetSourceEngineName(), "doi") == 0)
				{
					// Insurgency and Day of Infamy (presumably?) doesn't allow spaces in map names
					// and do use a space to delimit the map name from the map mode
					int i = 0;
					while (ptr[i] != 0)
					{
						if (ptr[i] == ' ')
						{
							ptr[i] = 0;
							break;
						}
						++i;
					}
				}

				if (!gamehelpers->IsMapValid(ptr))
				{
					continue;
				}

				if ((blk = pMapList->pArray->push()) != NULL)
				{
					strncopy((char *)blk, ptr, 255);
				}
			}

			bridge->filesystem->Close(fp);

			pMapList->last_modified_time = last_time;
			pMapList->serial = ++m_nSerialChange;
		}

		if (pMapList->pArray == NULL || pMapList->pArray->size() == 0)
		{
			return false;
		}

		*pSerial = pMapList->serial;
		*ppArray = pMapList->pArray;

		return true;
	}
	void DumpCache(List<maplist_info_t *> *compat_list)
	{
		m_ListLookup.clear();

		List<maplist_info_t *>::iterator iter = m_MapLists.begin();
		while (iter != m_MapLists.end())
		{
			if (compat_list != NULL && (*iter)->bIsCompat)
			{
				compat_list->push_back((*iter));
			}
			else
			{
				delete (*iter)->pArray;
				delete (*iter);
			}
			iter = m_MapLists.erase(iter);
		}
	}
private:
	char m_ConfigFile[PLATFORM_MAX_PATH];
	time_t m_ConfigLastChanged;
	ConVar *m_pMapCycleFile;
	NameHashSet<maplist_info_t *> m_ListLookup;
	List<maplist_info_t *> m_MapLists;
	MapListState m_CurState;
	unsigned int m_IgnoreLevel;
	maplist_info_t *m_pCurMapList;
	int m_nSerialChange;
} s_MapLists;

static cell_t LoadMapList(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	Handle_t hndl;
	cell_t *addr, flags;
	ICellArray *pArray, *pNewArray;

	hndl = params[1];
	pContext->LocalToPhysAddr(params[2], &addr);
	pContext->LocalToString(params[3], &str);
	flags = params[4];

	/* Make sure the input Handle is valid */
	pArray = NULL;
	if (hndl != BAD_HANDLE)
	{
		HandleError err;
		HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

		if ((err = handlesys->ReadHandle(hndl, htCellArray, &sec, (void **)&pArray))
			!= HandleError_None)
		{
			return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
		}
	}

	/* Make sure the map list cache is up to date at the root */
	s_MapLists.UpdateCache();

	/* Try to get the map list. */
	if ((pNewArray = s_MapLists.UpdateMapList(pArray, str, addr, flags)) == NULL)
	{
		return BAD_HANDLE;
	}

	/* If the user wanted a new array, create it now. */
	if (hndl == BAD_HANDLE)
	{
		if ((hndl = handlesys->CreateHandle(htCellArray, pNewArray, pContext->GetIdentity(), g_pCoreIdent, NULL))
			== BAD_HANDLE)
		{
			*addr = -1;
			delete pNewArray;
			return BAD_HANDLE;
		}
	}

	return hndl;
}

static cell_t SetMapListCompatBind(IPluginContext *pContext, const cell_t *params)
{
	char *name, *file;

	pContext->LocalToString(params[1], &name);
	pContext->LocalToString(params[2], &file);

	s_MapLists.UpdateCache();
	s_MapLists.AddOrUpdateDefault(name, file);

	return 1;
}

REGISTER_NATIVES(mapListNatives)
{
	{"ReadMapList",				LoadMapList},
	{"SetMapListCompatBind",	SetMapListCompatBind},
	{NULL,						NULL},
};
