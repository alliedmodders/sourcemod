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

#include <memory>
#include <utility>

#include "CRegEx.h"
#include "extension.h"

RegEx::RegEx()
{
	mErrorOffset = 0;
	mErrorCode = 0;
	re = nullptr;
	mAnchored = false;
	mFree = true;
	subject = nullptr;
}

void RegEx::Clear ()
{
	mErrorOffset = 0;
	mErrorCode = 0;
	mError.clear();
	if (re)
		pcre2_code_free(re);
	re = nullptr;
	mAnchored = false;
	mFree = true;
	if (subject)
		free(subject);
	subject = nullptr;
	mMatches.clear();
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

bool RegEx::Compile(const char *pattern, uint32_t iFlags)
{
	if (!mFree)
		Clear();

	// store if we have PCRE2_ANCHORED set so we can pass them into pcre2_match later to match pcre1 behavior
	if (iFlags & PCRE2_ANCHORED)
	{
		mAnchored = true;
	}
	re = pcre2_compile(reinterpret_cast<PCRE2_SPTR8>(pattern), PCRE2_ZERO_TERMINATED, iFlags, &mErrorCode, &mErrorOffset, nullptr);

	if (re == nullptr)
	{
		PCRE2_UCHAR buffer[256];
		int len = pcre2_get_error_message(mErrorCode, buffer, sizeof(buffer));
		mError.assign(reinterpret_cast<char *>(buffer), len);
		return false;
	}

	mFree = false;

	return true;
}

int RegEx::Match(const char *const str, const size_t offset)
{
	int rc = 0;

	if (mFree || re == nullptr)
		return -1;

	std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)> matchData (
		pcre2_match_data_create_from_pattern(re, nullptr),
		&pcre2_match_data_free
	);
	this->ClearMatch();

	//save str
	subject = strdup(str);

	uint32_t options = 0;
	if (mAnchored)
	{
		options |= PCRE2_ANCHORED;
	}
	rc = pcre2_match(re, reinterpret_cast<PCRE2_SPTR8>(subject), strlen(subject), offset, options, matchData.get(), nullptr);

	if (rc < 0)
	{
		if (rc == PCRE2_ERROR_NOMATCH)
		{
			return 0;
		} else {
			mErrorCode = rc;
			return -1;
		}
	}

	PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchData.get());
	uint32_t ovectorCount = pcre2_get_ovector_count(matchData.get());
	RegexMatch match;
	match.mSubStringCount = rc;
	match.mVector.reserve(ovectorCount);
	for (int i = 0; i < rc; i++)
	{
		match.mVector.emplace_back(RegexOffsetPair{ .start = ovector[2 * i], .end = ovector[2* i + 1] });
	}
	mMatches.push_back(std::move(match));

	return 1;
}

int RegEx::MatchAll(const char *str)
{
	int rc = 0;

	if (mFree || re == nullptr)
		return -1;

	std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)> matchData (
		pcre2_match_data_create_from_pattern(re, nullptr),
		&pcre2_match_data_free
	);
	this->ClearMatch();

	//save str
	subject = strdup(str);
	size_t len = strlen(subject);

	size_t offset = 0;
	uint32_t options = 0;
	if (mAnchored)
	{
		options |= PCRE2_ANCHORED;
	}

	while (offset < len && (rc = pcre2_match(re, reinterpret_cast<PCRE2_SPTR8>(subject), len, offset, options,
		matchData.get(), nullptr)) >= 0)
	{
		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchData.get());
		uint32_t ovectorCount = pcre2_get_ovector_count(matchData.get());
		offset = ovector[1];

		RegexMatch match;
		match.mSubStringCount = rc;
		match.mVector.reserve(ovectorCount);
		for (int i = 0; i < rc; i++)
		{
			match.mVector.emplace_back(RegexOffsetPair{ .start = ovector[2 * i], .end = ovector[2 * i + 1] });
		}
		mMatches.push_back(std::move(match));
	}

	if (rc < PCRE2_ERROR_NOMATCH || (rc == PCRE2_ERROR_NOMATCH && mMatches.empty()))
	{
		if (rc == PCRE2_ERROR_NOMATCH)
		{
			return 0;
		}
		else {
			mErrorCode = rc;
			return -1;
		}
	}

	return 1;
}

void RegEx::ClearMatch()
{
	// Clears match results
	mErrorOffset = 0;
	mErrorCode = 0;
	mError.clear();
	if (subject)
		free(subject);
	subject = nullptr;
	mMatches.clear();
}

bool RegEx::GetSubstring(int s, char buffer[], int max, size_t match)
{
	int i = 0;

	if (match >= mMatches.size())
		return false;

	if (s >= mMatches[match].mSubStringCount || s < 0)
		return false;

	char *substr_a = subject + mMatches[match].mVector[s].start;
	size_t substr_l = mMatches[match].mVector[s].end - mMatches[match].mVector[s].start;

	for (i = 0; i<substr_l; i++)
	{
		if (i >= max)
			break;
		buffer[i] = substr_a[i];
	}

	buffer[i] = '\0';

	return true;
}

