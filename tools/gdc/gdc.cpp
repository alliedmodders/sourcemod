#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <math.h>
#include "gdc.h"
#include "GameConfigs.h"
#include "MemoryUtils.h"

MemoryUtils mu;

char *game = NULL;
char *engine = NULL;
char *binary = NULL;

int main(int argc, char **argv)
{
	char *gamedata = NULL;
	bool debug = false;

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "b:de:f:g:")) != -1)
		switch (opt)
		{
			case 'd':
				debug = true;
				break;
			case 'g':
				game = optarg;
				break;

			case 'e':
				engine = optarg;
				break;

			case 'f':
				gamedata = optarg;
				break;

			case 'b':
				binary = optarg;
				break;

			case '?':
				printf("Usage: ./gdc -g <game> -e <engine> -f <gamedata file> -b <binary>\n");
				return 0;

			default:
				printf("WHAT\n");
				return 0;
		}

	if (!game || !engine || !gamedata || !binary)
	{
		printf("Usage: ./gdc -g <game> -e <engine> -f <gamedata file> -b <binary>\n");
		return 0;
	}

	printf("Game: %s\nEngine: %s\nGamedata: %s\nBinary: %s\n", game, engine, gamedata, binary);

	void *handle = dlopen(binary, RTLD_LAZY);
	if (!handle)
	{
		printf("Couldn't open %s (%s)\n", binary, dlerror());
		return 0;
	}

	CGameConfig gc;
	char err[512];
	if (!gc.EnterFile(gamedata, err, sizeof(err)))
	{
		printf("%s: %s\n", gamedata, err);
		return 0;
	}

	CGameConfig symbols;
	if (!symbols.EnterFile("symbols.txt", err, sizeof(err)))
	{
		printf("symbols.txt: %s\n", err);
		return 0;
	}

	if (debug)
		for (map<const char*, const char*, cmp_str>::iterator it = symbols.m_Keys.begin(); it != symbols.m_Keys.end(); it++)
			printf("%s - %s\n", it->first, it->second);

	const char *vtsym = symbols.GetKeyValue("vtsym");
	if (!vtsym)
	{
		printf("Couldn't find vtsym\n");
		return 0;
	}

#if 0
	void *lolwhat = (void*)1;
	void *handle = lolwhat;
	handle = dlopen(binary, RTLD_LAZY);
	if (!handle)
	{
		printf("Couldn't open %s\n", binary);
		return 0;
	}
#endif

	void **vt = (void**) mu.ResolveSymbol(handle, vtsym);
	if (!vt)
	{
		printf("Couldn't find vtable %s\n", vtsym);
		dlclose(handle);
		return 0;
	}


	for (list<Offset>::iterator it = gc.m_Offsets.begin(); it != gc.m_Offsets.end(); it++)
	{
		if (debug) printf("DEBUG %s - l: %d w: %d\n", it->name, it->lin, it->win);
//		else
		{
			const char *symbol = symbols.GetKeyValue(it->name);
			if (symbol)
			{
				int offset = findVFunc(handle, vt, symbol);

				if (offset == it->lin) printf("%s - good\n", it->name);
				else printf("%s - l: %d w: %d\n", it->name, offset, offset + it->lin - it->win);
			}
			else printf("%s - no Linux symbol, skipping\n", it->name);
		}
	}

	printf("\nWindows offsets are wild guesses!\n\n");

	for (list<Sig>::iterator it = gc.m_Sigs.begin(); it != gc.m_Sigs.end(); it++)
	{
		if (debug) printf("DEBUG %s - %s - l: %s\n", it->name, it->lib == Server ? "server" : "engine", it->lin);
//		else
		{
			if (it->lib != Server)
			{
				printf("%s - isn't server, skipping\n", it->name);
				continue;
			}

			const char *symbol = it->lin;

			if (!symbol) 
			{
				printf("%s - no Linux symbol, skipping\n", it->name);
				continue;
			}

			if (symbol[0] != '@')
			{
				printf("%s - didn't start with '@', skipping\n", it->name);
				continue;
			}

			void *addr = mu.ResolveSymbol(handle, symbol + 1);

			if (addr) printf("%s - good\n", it->name);
			else printf("%s - %s not found\n", it->name, it->lin);
		}
	}

	return 0;
}

/* 
   takes a mangled member function symbol and returns the position where the function name and parameters start
   01234567890123456789012345678
   _ZN9CTFPlayer12ForceRespawnEv
                ^13            ^28
*/
void findFuncPos(const char *sym, int &func, int &params)
{
	int i = 0;
	while ((sym[i] < '0') || (sym[i] > '9')) i++;
	int classLen = atoi(sym + i);
	func = i + (int)ceil(log10(classLen)) + classLen;
	int funcLen = atoi(sym + func);
	params = func + (int)ceil(log10(funcLen)) + funcLen;
}

int findVFunc(void *handle, void **vt, const char *symbol)
{
	int offset = 1;
	//int overloads = 0;
	int funcPos, paramPos;
	findFuncPos(symbol, funcPos, paramPos);

	for (int i = 0; i < 1000; i++)
	{
		void *pFunc = vt[i];
		const char *s;

//		Dl_info info;
//		if (!mu.ResolveAddr(pFunc, &info)) continue;
		if (!(s = mu.ResolveAddr(handle, pFunc))) continue;

//		if ((i > 1) && (strncmp(info.dli_sname, "_ZTI", 4) == 0)) break;
		if ((i > 1) && (strncmp(s, "_ZTI", 4) == 0)) break;


		int tempFuncPos, tempParamPos;
//		findFuncPos(info.dli_sname, tempFuncPos, tempParamPos);
		findFuncPos(s, tempFuncPos, tempParamPos);

#if 0
		if (strncmp(info.dli_sname[tempFuncPos], symbol[funcPos], paramPos - funcPos) == 0)
		{
			overloads++;
			if (firstOverload == -1) firstOverload = i - 1;
			if (strncmp(info.dli_sname[tempParamPos], symbol[paramPos], strlen(symbol) - paramPos) == 0)
			{
				offset = i;
				matchingOverload = overloads;
			}
		}
#else
//		if (strcmp(info.dli_sname + tempFuncPos, symbol + funcPos) == 0)
		if (strcmp(s + tempFuncPos, symbol + funcPos) == 0)
		{
			offset = i;
			break;
		}
#endif
	}

	return offset - 2;
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

