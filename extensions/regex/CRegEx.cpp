/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Regular Expressions Extension
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

#define PCRE_STATIC

#include "pcre.h"
#include "CRegEx.h"
#include "extension.h"

RegEx::RegEx()
{
	mErrorOffset = 0;
	mError = NULL;
	re = NULL;
	mFree = true;
	subject = NULL;
	mSubStrings = 0;
}

void RegEx::Clear ()
{
	mErrorOffset = 0;
	mError = NULL;
	if (re)
		pcre_free(re);
	re = NULL;
	mFree = true;
	if (subject)
		delete [] subject;
	subject = NULL;
	mSubStrings = 0;
}

RegEx::~RegEx()
{
	Clear();
}

bool RegEx::isFree(bool set, bool val)
{
	if (set)
	{
		mFree = val;
		return true;
	} else {
		return mFree;
	}
}

int RegEx::Compile(const char *pattern, int iFlags)
{
	if (!mFree)
		Clear();
		
	re = pcre_compile(pattern, iFlags, &mError, &mErrorOffset, NULL);

	if (re == NULL)
	{
		return 0;
	}

	mFree = false;

	return 1;
}

int RegEx::Match(const char *str)
{
	int rc = 0;

	if (mFree || re == NULL)
		return -1;
		
	this->ClearMatch();

	//save str
	subject = new char[strlen(str)+1];
	strcpy(subject, str);

	unsigned int offset = 0;
	unsigned int len = strlen(subject);
	unsigned int matches = 0;

	while (offset < len && (rc = pcre_exec(re, 0, subject, len, offset, 0, ovector, sizeof(ovector))) >= 0)
	{
		/* This also works for capture groups now, but assume rc is always 1 if we have any match.
		Reference : https://stackoverflow.com/questions/1421785/how-can-i-use-pcre-to-get-all-match-groups
		Example:
		rc = 2 for
		tes(t)
		with string test

		0 = test
		1 = t

		for (int i = 0; i < rc; ++i)
		{
			printf("%2d: %.*s\n", i, ovector[2 * i + 1] - ovector[2 * i], str + ovector[2 * i]);
		}
		*/

		char *substr_a = subject + ovector[2 * 0];
		int substr_l = ovector[2 * 0 + 1] - ovector[2 * 0];

		mMatches[matches] = ke::AString(substr_a, substr_l);
		offset = ovector[1];
		matches++;
	}

	if (rc < 0 && matches == 0) // rc < 0 when there is no more matches, so if we have matches dont error (Maybe check if rc == PCRE_ERROR_NOMATCH and if we have matches?)
	{
		if (rc == PCRE_ERROR_NOMATCH)
		{
			return 0;
		} else {
			mErrorOffset = rc;
			return -1;
		}
	}

	mSubStrings = matches;

	return 1;
}
void RegEx::ClearMatch()
{
	// Clears match results
	mErrorOffset = 0;
	mError = NULL;
	if (subject)
		delete [] subject;
	subject = NULL;
	mSubStrings = 0;
}

const char *RegEx::GetSubstring(int s, char buffer[], int max)
{
	int i = 0;
	if (s >= mSubStrings || s < 0)
		return NULL;

	ke::SafeStrcpy(buffer, max, mMatches[s].chars());

	return buffer;
}

