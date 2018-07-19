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

#include "common_logic.h"
#include "PhraseCollection.h"
#include "Translator.h"
#include "sprintf.h"
#include <am-string.h>

CPhraseCollection::CPhraseCollection()
{
}

CPhraseCollection::~CPhraseCollection()
{
}

void CPhraseCollection::Destroy()
{
	delete this;
}

IPhraseFile *CPhraseCollection::AddPhraseFile(const char *filename)
{
	size_t i;
	unsigned int fid;
	IPhraseFile *pFile;
	char full_name[PLATFORM_MAX_PATH];

	/* No compat shim here.  The user should have read the doc. */
	ke::SafeSprintf(full_name, sizeof(full_name), "%s.txt", filename);
	
	fid = g_Translator.FindOrAddPhraseFile(full_name);
	pFile = g_Translator.GetFileByIndex(fid);

	for (i = 0; i < m_Files.size(); i++)
	{
		if (m_Files[i] == pFile)
		{
			return pFile;
		}
	}

	m_Files.push_back(pFile);

	return pFile;
}

unsigned int CPhraseCollection::GetFileCount()
{
	return (unsigned int)m_Files.size();
}

IPhraseFile *CPhraseCollection::GetFile(unsigned int file)
{
	if (file >= m_Files.size())
	{
		return NULL;
	}

	return m_Files[file];
}

bool CPhraseCollection::TranslationPhraseExists(const char *key)
{
	for (size_t i = 0; i < m_Files.size(); i++)
	{
		if (m_Files[i]->TranslationPhraseExists(key))
		{
			return true;
		}
	}

	return false;
}

TransError CPhraseCollection::FindTranslation(const char *key, unsigned int langid, Translation *pTrans)
{
	size_t i;

	for (i = 0; i < m_Files.size(); i++)
	{
		if (m_Files[i]->GetTranslation(key, langid, pTrans) == Trans_Okay)
		{
			return Trans_Okay;
		}
	}

	return Trans_BadPhrase;
}

bool CPhraseCollection::FormatString(char *buffer,
								 size_t maxlength,
								 const char *format,
								 void **params,
								 unsigned int numparams,
								 size_t *pOutLength,
								 const char **pFailPhrase)
{
	unsigned int arg;

	arg = 0;
	if (!gnprintf(buffer, maxlength, format, this, params, numparams, arg, pOutLength, pFailPhrase))
	{
		return false;
	}

	if (arg != numparams)
	{
		if (pFailPhrase != NULL)
		{
			*pFailPhrase = NULL;
		}
		return false;
	}

	return true;
}
