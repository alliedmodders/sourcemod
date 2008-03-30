#ifndef _INCLUDE_CREGEX_H
#define _INCLUDE_CREGEX_H

class RegEx
{
public:
	RegEx();
	~RegEx();
	bool isFree(bool set=false, bool val=false);
	void Clear();

	int Compile(const char *pattern, int iFlags);
	int Match(const char *str);
	void ClearMatch();
	const char *GetSubstring(int s, char buffer[], int max);
public:
	int mErrorOffset;
	const char *mError;
	int mSubStrings;
private:
	pcre *re;
	bool mFree;
	int ovector[30];
	char *subject;
};

#endif //_INCLUDE_CREGEX_H

