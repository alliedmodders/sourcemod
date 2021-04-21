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

#ifndef _INCLUDE_SOURCEMOD_PHRASECOLLECTION_H_
#define _INCLUDE_SOURCEMOD_PHRASECOLLECTION_H_

#include <string.h>
#include <sh_vector.h>
#include <ITranslator.h>

using namespace SourceHook;
using namespace SourceMod;

class CPhraseCollection : public IPhraseCollection
{
public:
	CPhraseCollection();
	~CPhraseCollection();
public:
	IPhraseFile *AddPhraseFile(const char *filename);
	unsigned int GetFileCount();
	IPhraseFile *GetFile(unsigned int file);
	void Destroy();
	bool TranslationPhraseExists(const char *key);
	TransError FindTranslation(const char *key, unsigned int langid, Translation *pTrans);
	bool FormatString(
		char *buffer,
		size_t maxlength,
		const char *format,
		void **params,
		unsigned int numparams,
		size_t *pOutLength,
		const char **pFailPhrase);
private:
	CVector<IPhraseFile *> m_Files;
};

#endif //_INCLUDE_SOURCEMOD_PHRASECOLLECTION_H_
