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
	mErrorCode = 0;
	mError = nullptr;
	re = nullptr;
	mFree = true;
	subject = nullptr;
	mMatchCount = 0;
}

void RegEx::Clear ()
{
	mErrorOffset = 0;
	mErrorCode = 0;
	mError = nullptr;
	if (re)
		pcre_free(re);
	re = nullptr;
	mFree = true;
	if (subject)
		delete [] subject;
	subject = nullptr;
	mMatchCount = 0;
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
		
	re = pcre_compile2(pattern, iFlags, &mErrorCode, &mError, &mErrorOffset, nullptr);

	if (re == nullptr)
	{
		return 0;
	}

	mFree = false;

	return 1;
}

int RegEx::Match(const char *const str, const size_t offset)
{
	int rc = 0;

	if (mFree || re == nullptr)
		return -1;
		
	this->ClearMatch();

	//save str
	subject = strdup(str);

	rc = pcre_exec(re, nullptr, subject, strlen(subject), offset, 0, mMatches[0].mVector, MAX_CAPTURES);

	if (rc < 0)
	{
		if (rc == PCRE_ERROR_NOMATCH)
		{
			return 0;
		} else {
			mErrorCode = rc;
			return -1;
		}
	}

	mMatches[0].mSubStringCount = rc;
	mMatchCount = 1;

	return 1;
}

int RegEx::MatchAll(const char *str)
{
	int rc = 0;

	if (mFree || re == nullptr)
		return -1;

	this->ClearMatch();

	//save str
	subject = strdup(str);
	size_t len = strlen(subject);

	size_t offset = 0;
	unsigned int matches = 0;

	while (matches < MAX_MATCHES && offset < len && (rc = pcre_exec(re, 0, subject, len, offset, 0, mMatches[matches].mVector, MAX_CAPTURES)) >= 0)
	{
		offset = mMatches[matches].mVector[1];
		mMatches[matches].mSubStringCount = rc;

		matches++;
	}

	if (rc < PCRE_ERROR_NOMATCH || (rc == PCRE_ERROR_NOMATCH && matches == 0))
	{
		if (rc == PCRE_ERROR_NOMATCH)
		{
			return 0;
		}
		else {
			mErrorCode = rc;
			return -1;
		}
	}

	mMatchCount = matches;

	return 1;
}

void RegEx::ClearMatch()
{
	// Clears match results
	mErrorOffset = 0;
	mErrorCode = 0;
	mError = nullptr;
	if (subject)
		delete [] subject;
	subject = nullptr;
	mMatchCount = 0;
}

bool RegEx::GetSubstring(int s, char buffer[], int max, int match)
{
	int i = 0;

	if (s >= mMatches[match].mSubStringCount || s < 0)
		return false;

	char *substr_a = subject + mMatches[match].mVector[2 * s];
	int substr_l = mMatches[match].mVector[2 * s + 1] - mMatches[match].mVector[2 * s];

	for (i = 0; i<substr_l; i++)
	{
		if (i >= max)
			break;
		buffer[i] = substr_a[i];
	}

	buffer[i] = '\0';

	return true;
}

