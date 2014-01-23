/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Updater Extension
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

#include <sourcemod_version.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "extension.h"
#include "Updater.h"
#include <sh_list.h>
#include <sh_string.h>

#define DEFAULT_UPDATE_URL			"http://www.sourcemod.net/update/"

using namespace SourceHook;

SmUpdater g_Updater;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_Updater);

IWebternet *webternet;
static List<String *> update_errors;
static IThreadHandle *update_thread;
static String update_url;

bool SmUpdater::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "webternet.ext", true, true);

	SM_GET_IFACE(WEBTERNET, webternet);

	const char *url = smutils->GetCoreConfigValue("AutoUpdateURL");
	if (url == NULL)
	{
		url = DEFAULT_UPDATE_URL;
	}
	update_url.assign(url);
	
	ThreadParams params;
	params.flags = Thread_Default;
	params.prio = ThreadPrio_Low;
	update_thread = threader->MakeThread(this, &params);

	if (update_thread == NULL)
	{
		smutils->Format(error, maxlength, "Could not create thread");
		return false;
	}

	return true;
}

void SmUpdater::SDK_OnUnload()
{
	/* An interface drop might have killed this thread.  
	 * But if the handle is still there, we have to wait.
	 */
	if (update_thread != NULL)
	{
		update_thread->WaitForThread();
		update_thread->DestroyThis();
	}

	/* Clear message tables */
	List<String *>::iterator iter = update_errors.begin();
	while (iter != update_errors.end())
	{
		iter = update_errors.erase(iter);
	}
}

bool SmUpdater::QueryInterfaceDrop(SourceMod::SMInterface *pInterface)
{
	if (pInterface == webternet)
	{
		return false;
	}

	return true;
}

void SmUpdater::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == webternet)
	{
		/* Can't be in the thread if we're losing this extension. */
		update_thread->WaitForThread();
		update_thread->DestroyThis();
		update_thread = NULL;
	}
}

static void PumpUpdate(void *data)
{
	String *str;
	bool new_files = false;
	List<String *>::iterator iter;

	char path[PLATFORM_MAX_PATH];
	UpdatePart *temp;
	UpdatePart *part = (UpdatePart*)data;
	while (part != NULL)
	{
		if (strstr(part->file, "..") != NULL)
		{
			/* Naughty naughty */
			AddUpdateError("Detected invalid path escape (..): %s", part->file);
			goto skip_create;
		}
		if (part->data == NULL)
		{
			smutils->BuildPath(Path_SM, path, sizeof(path), "gamedata/%s", part->file);
			if (libsys->IsPathDirectory(path))
			{
				goto skip_create;
			}
			if (!libsys->CreateFolder(path))
			{
				AddUpdateError("Could not create folder: %s", path);
			}
			else
			{
				smutils->LogMessage(myself, "Created folder \"%s\" from updater", path);
			}
		}
		else
		{
			smutils->BuildPath(Path_SM, path, sizeof(path), "gamedata/%s", part->file);
			FILE *fp = fopen(path, "wb");
			if (fp == NULL)
			{
				AddUpdateError("Could not open %s for writing", path);
				goto skip_create;
			}
			if (fwrite(part->data, 1, part->length, fp) != part->length)
			{
				AddUpdateError("Could not write file %s", path);
			}
			else
			{
				smutils->LogMessage(myself,
					"Successfully updated gamedata file \"%s\"",
					part->file);
				new_files = true;
			}
			fclose(fp);
		}
skip_create:
		temp = part->next;
		free(part->data);
		free(part->file);
		delete part;
		part = temp;
	}

	if (update_errors.size())
	{
		smutils->LogError(myself, "--- BEGIN ERRORS FROM AUTOMATIC UPDATER ---");

		for (iter = update_errors.begin();
			 iter != update_errors.end();
			 iter++)
		{
			str = (*iter);
			smutils->LogError(myself, "%s", str->c_str());
		}

		smutils->LogError(myself, "--- END ERRORS FROM AUTOMATIC UPDATER ---");
	}

	if (new_files)
	{
		const char *force_restart = smutils->GetCoreConfigValue("ForceRestartAfterUpdate");
		if (force_restart == NULL || strcasecmp(force_restart, "yes") != 0)
		{
			smutils->LogMessage(myself,
				"SourceMod has been updated, please reload it or restart your server.");
		}
		else
		{
			char buffer[255];
			smutils->Format(buffer,
				sizeof(buffer),
				"meta unload %d\n",
				smutils->GetPluginId());
			gamehelpers->ServerCommand(buffer);
			smutils->Format(buffer,
				sizeof(buffer),
				"changelevel \"%s\"\n",
				gamehelpers->GetCurrentMap());
			gamehelpers->ServerCommand(buffer);
			gamehelpers->ServerCommand("echo SourceMod has been restarted from an automatic update.\n");
		}
	}
}

void SmUpdater::RunThread(IThreadHandle *pHandle)
{
	UpdateReader ur;

	ur.PerformUpdate(update_url.c_str());

	smutils->AddFrameAction(PumpUpdate, ur.DetachParts());
}

void SmUpdater::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
}

void AddUpdateError(const char *fmt, ...)
{
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	smutils->FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	update_errors.push_back(new String(buffer));
}

const char *SmUpdater::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *SmUpdater::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

