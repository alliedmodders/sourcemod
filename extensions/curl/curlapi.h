/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
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

#ifndef _INCLUDE_SOURCEMOD_CURLAPI_IMPL_H_
#define _INCLUDE_SOURCEMOD_CURLAPI_IMPL_H_

#include <IWebternet.h>
#include <curl/curl.h>

using namespace SourceMod;

class WebForm : public IWebForm
{
public:
	WebForm();
	~WebForm();
public:
	bool AddString(const char *name, const char *data);
	bool AddFile(const char *name, const char *path);
public:
	curl_httppost *GetFormData();
private:
	curl_httppost *first;
	curl_httppost *last;
	CURLFORMcode lastError;
};

class WebTransfer : public IWebTransfer
{
public:
	WebTransfer(CURL *curl);
	~WebTransfer();
	static WebTransfer *CreateWebSession();
public:
	const char *LastErrorMessage();
	int LastErrorCode();
	bool SetHeaderReturn(bool recv_hdr);
	bool Download(const char *url, ITransferHandler *handler, void *data);
	bool SetFailOnHTTPError(bool fail);
	bool PostAndDownload(const char *url,
		IWebForm *form,
		ITransferHandler *handler,
		void *data);
private:
	CURL *curl;
	char errorBuffer[CURL_ERROR_SIZE];
	CURLcode lastError;
};

class Webternet : public IWebternet
{
public:
	unsigned int GetInterfaceVersion();
	const char *GetInterfaceName();
public:
	IWebTransfer *CreateSession();
	IWebForm *CreateForm();
};

extern Webternet g_webternet;

#endif /* _INCLUDE_SOURCEMOD_CURLAPI_IMPL_H_ */

