#ifndef _INCLUDE_GAMEDATAMD5_MAIN_H_
#define _INCLUDE_GAMEDATAMD5_MAIN_H_

#include "TextParsers.h"
#include "sm_memtable.h"
#include "md5.h"

using namespace SourceMod;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

class BuildMD5ableBuffer : public ITextListener_SMC
{
public:

	BuildMD5ableBuffer()
	{
		stringTable = new BaseStringTable(2048);
		md5[0] = 0;
		md5String[0] = 0;
	}

	~BuildMD5ableBuffer()
	{
		delete stringTable;
	}

	void ReadSMC_ParseStart()
	{
		checksum = MD5();
	}

	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		stringTable->AddString(key);
		stringTable->AddString(value);

		return SMCResult_Continue;
	}

	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		stringTable->AddString(name);

		return SMCResult_Continue;
	}

	void ReadSMC_ParseEnd(bool halted, bool failed)
	{
		if (halted || failed)
		{
			return;
		}

		void *data = stringTable->GetMemTable()->GetAddress(0);

		if (data != NULL)
		{
			checksum.update((unsigned char *)data, stringTable->GetMemTable()->GetActualMemUsed());
		}

		checksum.finalize();

		checksum.hex_digest(md5String);
		checksum.raw_digest(md5);

		stringTable->Reset();
	}

	unsigned char * GetMD5()
	{
		return md5;
	}

	unsigned char * GetMD5String()
	{
		return (unsigned char *)&md5String[0];
	}

private:
	MD5 checksum;
	unsigned char md5[16];
	char md5String[33];
	BaseStringTable *stringTable;
};


#endif // _INCLUDE_GAMEDATAMD5_MAIN_H_
