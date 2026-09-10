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
#ifndef _INCLUDE_CREGEX_H
#define _INCLUDE_CREGEX_H

#include <string>
#include <am-string.h>

#include "pcre2.h"

#define MAX_MATCHES 20
#define MAX_CAPTURES MAX_MATCHES*3

struct RegexOffsetPair
{
	size_t start;
	size_t end;
};

struct RegexMatch
{
	int mSubStringCount;
	std::vector<RegexOffsetPair> mVector;
};

class RegEx
{
public:
	RegEx();
	~RegEx();
	bool isFree(bool set=false, bool val=false);
	void Clear();

	bool Compile(const char *pattern, uint32_t iFlags);
	int Match(const char *const str, const size_t offset);
	int MatchAll(const char *str);
	void ClearMatch();
	bool GetSubstring(int s, char buffer[], int max, size_t match);
public:
	PCRE2_SIZE mErrorOffset;
	int mErrorCode;
	std::string mError;
	std::vector<RegexMatch> mMatches;
private:
	pcre2_code *re;
	bool mAnchored;
	bool mFree;
	char *subject;
};

#endif //_INCLUDE_CREGEX_H

