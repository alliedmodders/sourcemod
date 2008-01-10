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

void RegEx::Clear()
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

