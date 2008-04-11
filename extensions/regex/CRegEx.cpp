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

#include "pcre.h"
#include "CRegEx.h"
#include <sh_string.h>
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

	rc = pcre_exec(re, NULL, subject, (int)strlen(subject), 0, 0, ovector, 30);

	if (rc < 0)
	{
		if (rc == PCRE_ERROR_NOMATCH)
		{
			return 0;
		} else {
			mErrorOffset = rc;
			return -1;
		}
	}

	mSubStrings = rc;

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

	char *substr_a = subject + ovector[2*s];
	int substr_l = ovector[2*s+1] - ovector[2*s];

	for (i = 0; i<substr_l; i++)
	{
		if (i >= max)
			break;
		buffer[i] = substr_a[i];
	}

	buffer[i] = '\0';

	return buffer;
}

