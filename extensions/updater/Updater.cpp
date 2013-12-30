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

#include <stdlib.h>
#include "extension.h"
#include "Updater.h"
#include "md5.h"
#include <sourcemod_version.h>

#define USTATE_NONE			0
#define USTATE_FOLDERS		1
#define USTATE_CHANGED		2
#define USTATE_CHANGE_FILE	3
#define USTATE_ERRORS		4

using namespace SourceMod;

UpdateReader::UpdateReader() : partFirst(NULL), partLast(NULL)
{
}

UpdateReader::~UpdateReader()
{
}

void UpdateReader::ReadSMC_ParseStart()
{
	ignoreLevel = 0;
	ustate = USTATE_NONE;
}

SMCResult UpdateReader::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (ignoreLevel)
	{
		ignoreLevel++;
		return SMCResult_Continue;
	}

	switch (ustate)
	{
	case USTATE_NONE:
		{
			if (strcmp(name, "Folders") == 0)
			{
				ustate = USTATE_FOLDERS;
			}
			else if (strcmp(name, "Changed") == 0)
			{
				ustate = USTATE_CHANGED;
			}
			else if (strcmp(name, "Errors") == 0)
			{
				ustate = USTATE_ERRORS;
			}
			else
			{
				ignoreLevel++;
			}
			break;
		}
	case USTATE_FOLDERS:
	case USTATE_CHANGE_FILE:
		{
			ignoreLevel++;
			break;
		}
	case USTATE_CHANGED:
		{
			curfile.assign(name);
			url.clear();
			checksum[0] = '\0';
			ustate = USTATE_CHANGE_FILE;
			break;
		}
	}

	return SMCResult_Continue;
}

SMCResult UpdateReader::ReadSMC_KeyValue(const SMCStates *states,
										 const char *key,
										 const char *value)
{
	if (ignoreLevel)
	{
		return SMCResult_Continue;
	}

	switch (ustate)
	{
	case USTATE_CHANGE_FILE:
		{
			if (strcmp(key, "md5sum") == 0)
			{
				if (strlen(value) != 32)
				{
					return SMCResult_Continue;
				}
				strcpy(checksum, value);
			}
			else if (strcmp(key, "location") == 0)
			{
				url.assign(update_url);
				url.append(value);
			}
			break;
		}
	case USTATE_ERRORS:
		{
			if (strcmp(key, "error") == 0)
			{
				AddUpdateError("%s", value);
			}
			break;
		}
	case USTATE_FOLDERS:
		{
			HandleFolder(value);
			break;
		}
	}

	return SMCResult_Continue;
}

SMCResult UpdateReader::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (ignoreLevel)
	{
		ignoreLevel--;
		return SMCResult_Continue;
	}

	switch (ustate)
	{
	case USTATE_FOLDERS:
	case USTATE_CHANGED:
	case USTATE_ERRORS:
		{
			ustate = USTATE_NONE;
			break;
		}
	case USTATE_CHANGE_FILE:
		{
			if (url.size() != 0 && checksum[0] != '\0')
			{
				HandleFile();
			}
			else
			{
				AddUpdateError("Incomplete file definition in update manifest");
			}
			ustate = USTATE_CHANGED;
			break;
		}
	}

	return SMCResult_Continue;
}

void UpdateReader::LinkPart(UpdatePart *part)
{
	part->next = NULL;
	if (partFirst == NULL)
	{
		partFirst = part;
		partLast = part;
	}
	else
	{
		partLast->next = part;
		partLast = part;
	}
}

void UpdateReader::HandleFile()
{
	MD5 md5;
	char real_checksum[33];

	mdl.Reset();
	
	if (!xfer->Download(url.c_str(), &mdl, NULL))
	{
		AddUpdateError("Could not download \"%s\"", url.c_str());
		AddUpdateError("Error: %s", xfer->LastErrorMessage());
		return;
	}

	md5.update((unsigned char *)mdl.GetBuffer(), mdl.GetSize());
	md5.finalize();
	md5.hex_digest(real_checksum);

	if (mdl.GetSize() == 0)
	{
		AddUpdateError("Zero-length file returned for \"%s\"", curfile.c_str());
		return;
	}

	if (strcasecmp(checksum, real_checksum) != 0)
	{
		AddUpdateError("Checksums for file \"%s\" do not match:", curfile.c_str());
		AddUpdateError("Expected: %s Real: %s", checksum, real_checksum);
		return;
	}

	UpdatePart *part = new UpdatePart;
	part->data = (char*)malloc(mdl.GetSize());
	memcpy(part->data, mdl.GetBuffer(), mdl.GetSize());
	part->file = strdup(curfile.c_str());
	part->length = mdl.GetSize();
	LinkPart(part);
}

void UpdateReader::HandleFolder(const char *folder)
{
	UpdatePart *part = new UpdatePart;
	part->data = NULL;
	part->length = 0;
	part->file = strdup(folder);
	LinkPart(part);
}

static bool md5_file(const char *file, char checksum[33])
{
	MD5 md5;
	FILE *fp;
	long length;
	void *fdata;

	if ((fp = fopen(file, "rb")) == NULL)
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fdata = malloc(length);
	if (fread(fdata, 1, length, fp) != size_t(length))
	{
		free(fdata);
		fclose(fp);
		return false;
	}
	fclose(fp);

	md5.update((unsigned char*)fdata, length);
	md5.finalize();
	md5.hex_digest(checksum);

	free(fdata);

	return true;
}

/* Path should be sourcemod relative, not gamedata relative */
static bool add_file(IWebForm *form, const char *file, unsigned int &num_files)
{
	char path[PLATFORM_MAX_PATH];

	smutils->BuildPath(Path_SM, path, sizeof(path), "%s", file);

	char checksum[33];
	if (!md5_file(path, checksum))
	{
		return false;
	}

	char name[32];
	smutils->Format(name, sizeof(name), "file_%d_name", num_files);
	form->AddString(name, file);
	smutils->Format(name, sizeof(name), "file_%d_md5", num_files);
	form->AddString(name, checksum);

	num_files++;

	return true;
}

static void add_folders(IWebForm *form, const char *root, unsigned int &num_files)
{
	IDirectory *dir;
	char path[PLATFORM_MAX_PATH];
	char name[PLATFORM_MAX_PATH];

	smutils->BuildPath(Path_SM, path, sizeof(path), "%s", root);
	dir = libsys->OpenDirectory(path);
	if (dir == NULL)
	{
		AddUpdateError("Could not open folder: %s", path);
		return;
	}

	while (dir->MoreFiles())
	{
		if (strcmp(dir->GetEntryName(), ".") == 0 ||
			strcmp(dir->GetEntryName(), "..") == 0)
		{
			dir->NextEntry();
			continue;
		}
		smutils->Format(name, sizeof(name), "%s/%s", root, dir->GetEntryName());
		if (dir->IsEntryDirectory())
		{
			add_folders(form, name, num_files);
		}
		else if (dir->IsEntryFile())
		{
			add_file(form, name, num_files);
		}
		dir->NextEntry();
	}

	libsys->CloseDirectory(dir);
}

void UpdateReader::PerformUpdate(const char *url)
{
	IWebForm *form;
	MemoryDownloader master;
	SMCStates states = {0, 0};

	update_url = url;

	form = webternet->CreateForm();
	xfer = webternet->CreateSession();
	xfer->SetFailOnHTTPError(true);

	form->AddString("version", SOURCEMOD_VERSION);

	unsigned int num_files = 0;
	add_folders(form, "gamedata", num_files);

	char temp[24];
	smutils->Format(temp, sizeof(temp), "%d", num_files);
	form->AddString("files", temp);

	if (!xfer->PostAndDownload(url, form, &master, NULL))
	{
		AddUpdateError("Could not download \"%s\"", url);
		AddUpdateError("Error: %s", xfer->LastErrorMessage());
		goto cleanup;
	}
	
	SMCError error;
	char errbuf[256];
	error = textparsers->ParseSMCStream(master.GetBuffer(),
										master.GetSize(),
										this,
										&states,
										errbuf,
										sizeof(errbuf));
	if (error != SMCError_Okay)
	{
		AddUpdateError("Parse error in update manifest: %s", errbuf);
		goto cleanup;
	}

cleanup:
	delete xfer;
	delete form;
}

UpdatePart *UpdateReader::DetachParts()
{
	UpdatePart *first;

	first = partFirst;
	partFirst = NULL;
	partLast = NULL;

	return first;
}

