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

#include "TagsSystem.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "PluginSys.h"
#include "compat_wrappers.h"

TagHandler g_Tags;

TagHandler::TagHandler()
{
	m_SvTags = NULL;
}

TagHandler::~TagHandler()
{
	SourceHook::List<char *>::iterator iter = m_TagList.begin();

	while (iter != m_TagList.end())
	{
		g_pMemAlloc->Free(*iter);
		iter = m_TagList.erase(iter);
	}
}

void TagHandler::OnSourceModAllInitialized_Post()
{
	g_PluginSys.AddPluginsListener(this);

	m_SvTags = icvar->FindVar("sv_tags");
	AddTag("sourcemod");
}

void TagHandler::OnSourceModLevelActivated()
{
	SourceHook::List<char *>::iterator iter = m_TagList.begin();

	while (iter != m_TagList.end())
	{
		ApplyTag(*iter);
		iter++;
	}
}

void TagHandler::OnSourceModShutdown()
{
	RemoveTag("sourcemod");
}

void TagHandler::OnPluginDestroyed(IPlugin *plugin)
{
	SourceHook::List<char *> *pList = NULL;

	if (!plugin->GetProperty("ServerTags", (void **)&pList, false) || !pList)
	{
		return;
	}

	SourceHook::List<char *>::iterator iter = pList->begin();

	while (iter != pList->end())
	{
		RemoveTag(*iter);
		g_pMemAlloc->Free(*iter);
		iter = pList->erase(iter);
	}

	delete pList;
}

bool TagHandler::AddTag(const char *addTag)
{
	SourceHook::List<char *>::iterator iter = m_TagList.begin();

	while (iter != m_TagList.end())
	{
		if (strcmp(*iter, addTag) == 0)
		{
			return false;
		}
	}

	m_TagList.push_back(strdup(addTag));
	ApplyTag(addTag);

	return true;
}

bool TagHandler::RemoveTag(const char *removeTag)
{
	SourceHook::List<char *>::iterator iter = m_TagList.begin();

	while (iter != m_TagList.end())
	{
		if (strcmp(*iter, removeTag) == 0)
		{
			g_pMemAlloc->Free(*iter);
			m_TagList.erase(iter);
			StripTag(removeTag);

			return true;
		}
	}

	return false;
}

void TagHandler::ApplyTag(const char *addTag)
{
	if (m_SvTags == NULL)
	{
		return;
	}

	const char *curTags = m_SvTags->GetString();

	if (strstr(curTags, addTag) != NULL)
	{
		/* Already tagged */
		return;
	}

	if (curTags[0] == '\0')
	{
		m_SvTags->SetValue(addTag);
		return;
	}

	/* New tags buffer (+2 for , and null char) */
	size_t newLen = strlen(curTags) + strlen(addTag) + 2;
	char *newTags = (char *)alloca(newLen);

	g_SourceMod.Format(newTags, newLen, "%s,%s", curTags, addTag);

	m_SvTags->SetValue(newTags);
}

void TagHandler::StripTag(const char *removeTag)
{
	if (m_SvTags == NULL)
	{
		return;
	}

	const char *curTags = m_SvTags->GetString();

	if (strcmp(curTags, removeTag) == 0)
	{
		m_SvTags->SetValue("");
		return;
	}

	char *newTags = strdup(curTags);
	size_t searchLen = strlen(removeTag) + 2;
	char *search = (char *)alloca(searchLen);

	if (strncmp(curTags, removeTag, strlen(removeTag)) == 0)
	{
		strcpy(search, removeTag);
		search[searchLen-2] = ',';
		search[searchLen-1] = '\0';
	}
	else
	{
		strcpy(search+1, removeTag);
		search[0] = ',';
		search[searchLen-1] = '\0';
	}

	UTIL_ReplaceAll(newTags, strlen(newTags) + 1, search, "" , true);

	m_SvTags->SetValue(newTags);

	g_pMemAlloc->Free(newTags);
}
