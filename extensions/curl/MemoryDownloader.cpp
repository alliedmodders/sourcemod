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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "MemoryDownloader.h"

using namespace SourceMod;

MemoryDownloader::MemoryDownloader() : buffer(NULL), bufsize(0), bufpos(0)
{
}

MemoryDownloader::~MemoryDownloader()
{
	free(buffer);
}

DownloadWriteStatus MemoryDownloader::OnDownloadWrite(IWebTransfer *session,
													  void *userdata,
													  void *ptr,
													  size_t size,
													  size_t nmemb)
{
	size_t total = size * nmemb;

	if (bufpos + total > bufsize)
	{
		size_t rem = (bufpos + total) - bufsize;
		bufsize += rem + (rem / 2);
		buffer = (char *)realloc(buffer, bufsize);
	}

	assert(bufpos + total <= bufsize);

	memcpy(&buffer[bufpos], ptr, total);
	bufpos += total;

	return DownloadWrite_Okay;
}

void MemoryDownloader::Reset()
{
	bufpos = 0;
}

char *MemoryDownloader::GetBuffer()
{
	return buffer;
}

size_t MemoryDownloader::GetSize()
{
	return bufpos;
}

