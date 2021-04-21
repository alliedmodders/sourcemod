#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>
#include <link.h>
#include "gdc.h"
#include "GameConfigs.h"
#include "MemoryUtils.h"

MemoryUtils mu;

char *game = NULL;
char *engine = NULL;
char *game_binary = NULL;
char *engine_binary = NULL;
char *wgame_binary = NULL;
char *wengine_binary = NULL;
char *symbols_file = NULL;

bool use_symtab = true;
CGameConfig symbols;
CGameConfig gc;

inline bool IsDigit( char c )
{
	return c < 58 && c > 47;
}

void PrintUsage()
{
	printf("Usage: ./gdc -g <game> -e <engine name> -f <gamedata file> -b <game binary> -x <engine binary> -w <win game binary> -y <win engine binary> [-s <symbols.txt file>]\n");
	printf("Other options:\n");
	printf(" -d\tAdd debug output\n");
	printf(" -n\tDon't use symbol table for lookups (falls back to dlsym)\n");
}

int main(int argc, char **argv)
{
	char *gamedata = NULL;
	bool debug = false;

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "b:nde:f:g:x:w:y:s:")) != -1)
	{
		switch (opt)
		{
			case 'd':
				debug = true;
				break;

			case 'n':
				use_symtab = false;
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
				game_binary = optarg;
				break;

			case 'x':
				engine_binary = optarg;
				break;

			case 'w':
				wgame_binary = optarg;
				break;

			case 'y':
				wengine_binary = optarg;
				break;

			case 's':
				symbols_file = optarg;
				break;

			case '?':
				PrintUsage();
				return 0;

			default:
				printf("WHAT\n");
				return 0;
		}
	}

	if (!game || !engine || !gamedata || !game_binary || !engine_binary || !wgame_binary || !wengine_binary)
	{
		PrintUsage();
		return 0;
	}

	printf("Game: %s\n", game);
	if (debug)
	{
		printf("Engine: %s\nGame binary: %s\nEngine binary: %s\nWin game binary: %s\nWin engine binary: %s\n",
			engine, game_binary, engine_binary, wgame_binary, wengine_binary
			);
	}
	printf("Gamedata: %s\n\n", gamedata);

	void *ghandle;
	ghandle = dlopen(game_binary, RTLD_LAZY);
	if (!ghandle)
	{
		printf("Couldn't open %s (%s)\n", game_binary, dlerror());
			return 0;
	}
	void *ehandle;
	ehandle = dlopen(engine_binary, RTLD_LAZY);
	if (!ehandle)
	{
		printf("Couldn't open %s (%s)\n", engine_binary, dlerror());
			return 0;
	}

	int wgfile = open(wgame_binary, O_RDONLY);
	if (wgfile == -1)
	{
		printf("Couldn't open %s\n", wgame_binary);
		return 0;
	}

	int wefile = open(wengine_binary, O_RDONLY);
	if (wefile == -1)
	{
		printf("Couldn't open %s\n", wengine_binary);
		return 0;
	}

	char err[512];
	if (!gc.EnterFile(gamedata, err, sizeof(err)))
	{
		printf("%s: %s\n", gamedata, err);
		return 0;
	}

	if (!symbols.EnterFile(symbols_file ? symbols_file : "symbols.txt", err, sizeof(err)))
	{
		printf("symbols.txt: %s\n", err);
		return 0;
	}

	for (list<Offset>::iterator it = gc.m_Offsets.begin(); it != gc.m_Offsets.end(); it++)
	{
		if (debug)
		{
			printf("DEBUG %s - l: %d w: %d\n", it->name, it->lin, it->win);
		}

		const char *symbol = symbols.GetKeyValue(it->name);
		if (symbol)
		{
			char symvt[128] = "_ZTV";
			bool bGotFirstNumbers = false;
			bool bLastNumberDigit = false;
			unsigned int symlen = strlen(symbol);
			unsigned int pos = strlen(symvt);
			
			for ( unsigned int i = 0; i < symlen && pos < (sizeof(symvt)-1); ++i )
			{
				bool isDigit = IsDigit( symbol[i] );
				if( !isDigit && bLastNumberDigit )
				{
					bGotFirstNumbers = true;
				}
				
				// we're before the class len
				if( !bGotFirstNumbers && !isDigit )
					continue;
				
				// we're at the function name len
				if( bGotFirstNumbers && isDigit )
				{
					symvt[pos] = 0;
					break;
				}
				
				// we're at the class len or class name. we want these
				symvt[pos++] = symbol[i];
				if( isDigit )
				{
					bLastNumberDigit = isDigit;
				}
			}
			symvt[pos] = 0;
			
			int newOffset;
			if (symvt[0])
			{
                void **newvt = NULL;
				if( use_symtab )
				{
					newvt = (void**)mu.ResolveSymbol(ghandle, symvt);
				}
				else
				{
					newvt = (void **)dlsym(ghandle, symvt);
				}
				
			    if (!newvt)
				{
					printf("? O: %-22s - can't find, skipping\n", symvt);
					continue;
				}

				newOffset = findVFunc(ghandle, newvt, symbol);
			}

			if (newOffset == it->lin)
			{
				printf("  O: %-22s - GOOD.    current [ w: %3d, l: %3d ].\n", it->name, it->win, it->lin);
			}
			else
			{
				printf("! O: %-22s - CHANGED. old [ w: %3d, l: %3d ]. new [ w: %3d, l: %3d ].\n",
					it->name,
					it->win, it->lin,
					newOffset - (it->lin - it->win), newOffset
					);
			}
		}
		else // !symbol
		{
			printf("  O: %-22s - no Linux symbol, skipping\n", it->name);
		}
	}

	printf("\nWindows offsets are (semi-)wild guesses!\n\nSignature offsets are wild guesses!\n\n");

	for (list<Sig>::iterator it = gc.m_Sigs.begin(); it != gc.m_Sigs.end(); it++)
	{
		if (debug)
		{
			printf("DEBUG %s - %s - l: %s\n", it->name, it->lib == Server ? "server" : "engine", it->lin);
		}

		const char *linSymbol = it->lin;
		const char *winSymbol = it->win;
		bool hasLinux   = (linSymbol && linSymbol[0]);
		bool hasWindows = (winSymbol && winSymbol[0]);
		if (!hasLinux && !hasWindows)
		{
			printf("  S: %-22s - hasn't linux nor windows data, skipping\n", it->name);
			continue;
		}
		
		int  winFile;
		void *linHandle;
		if (it->lib == Server)
		{
			winFile   = wgfile;
			linHandle = ghandle;
		}
		else if (it->lib == Engine)
		{
			winFile   = wefile;
			linHandle = ehandle;
		}
		else
		{
			printf("  S: %-22s - isn't from server nor engine, skipping\n", it->name);
			continue;
		}
		
		int linuxMatches = 0, windowsMatches = 0;
		if( hasLinux )
			linuxMatches = checkSigStringL(linHandle, linSymbol);
		if( hasWindows )
			windowsMatches = checkSigStringW(winFile,   winSymbol);
			
		if (linuxMatches == 1 && windowsMatches == 1)
		{
			// too much clutter to print current data if it matches anyway, unlike with offsets
			/*
			printf("%-22s (%s) - GOOD. current:\n\t[w:%s]\n\t[l:%s]\n",
				it->name,
				(it->lib == Server) ? "server" : "engine",
				winSymbol ? winSymbol : "",
				linSymbol ? linSymbol : ""
				);
			*/
			printf("  S: %-22s (%s) - w: GOOD     - l: GOOD    \n",
				it->name,
				(it->lib == Server) ? "server" : "engine"
				);

			//Check if they signature has a matching offset
			CheckWindowsSigOffset(it->name, winSymbol, winFile);
			CheckLinuxSigOffset(it->name, linSymbol, linHandle);

		}
		else
		{
			char winStatus[16];
			// right now the only option is to ignore
			const char *options = symbols.GetOptionValue(it->name);
			bool allowMulti = ( options && strstr(options, "allowmultiple") != NULL );
			bool allowMidfunc = ( options && strstr(options, "allowmidfunc") != NULL );
			
			bool bWinGood = false;
			if( !hasWindows ) {
				snprintf( winStatus, sizeof(winStatus), "UNKNOWN" );
			}
			else if( windowsMatches == -1 && !allowMidfunc) {
				snprintf( winStatus, sizeof(winStatus), "MIDFUNC" );
			}
			else if( windowsMatches == 0 ) {
				snprintf( winStatus, sizeof(winStatus), "NOTFOUND" );
			}
			else if( windowsMatches > 1 && !allowMulti) {
				snprintf( winStatus, sizeof(winStatus), "MULTIPLE" );
			}
			else {
				bWinGood = true;
				snprintf( winStatus, sizeof(winStatus), "GOOD" );
			}
			
			char linStatus[16];
			bool bLinGood = false;
			if( !hasLinux ) {
				snprintf( linStatus, sizeof(linStatus), "UNKNOWN" );
			}
			else if( linuxMatches == 0 ) {
				snprintf( linStatus, sizeof(linStatus), "NOTFOUND" );
			}
			else if( linuxMatches == 1 ) {
				bLinGood = true;
				snprintf( linStatus, sizeof(linStatus), "GOOD" );
			}
			else {
				snprintf( linStatus, sizeof(linStatus), "CHECK" );
			}
			
			bool showWinExtra = (hasWindows && !bWinGood);
			bool showLinExtra = ( ( hasLinux && !bLinGood ) || showWinExtra );
			
			printf("%s S: %-22s (%s)", (showWinExtra || showLinExtra) ? "!" : " ", it->name, (it->lib == Server) ? "server" : "engine" );
				printf(" - w: %-8s - l: %-8s\n", winStatus, linStatus );
			
			if( showWinExtra || showLinExtra )
				printf( "!    current:\n" );
			
			if( showWinExtra )
			{
				printf("!    w: \"%s\"\n",
					winSymbol ? winSymbol : ""
					);
			}
			
			if( showLinExtra )
			{
				// extra \n at end is intentional to add buffer after possibly long sigs
				printf("!    l: \"%s\"\n\n",
					linSymbol ? linSymbol : ""
					);
			}			
		}
	}

	return 0;
}
void CheckWindowsSigOffset(char* name, const char* symbol, int file)
{
	void *ptr = GetWindowsSigPtr(file, symbol);
	if(!ptr)
	{
		return;
	}
	const char* sigOffsetKey = NULL;
	const char* sigOffsetByte = NULL;
	int sigOffset = -1;
	char sigOffsetName[128];
	char sigByteName[128];

	snprintf(sigOffsetName, sizeof(sigOffsetName), "%s_Offset", name);
	snprintf(sigByteName, sizeof(sigByteName), "%s_Byte_Win", name);

	sigOffsetKey = symbols.GetKeyValue((const char *)sigOffsetName);
	if(sigOffsetKey == NULL)
	{

		//Maybe it has multiple?
		for(unsigned int i = 1; i <= 4; i++)
		{
			snprintf(sigOffsetName, sizeof(sigOffsetName), "%s_Offset%i", name, i);
			snprintf(sigByteName, sizeof(sigByteName), "%s_Byte_Win%i", name, i);
			sigOffsetKey = symbols.GetKeyValue((const char *)sigOffsetName);

			if(sigOffsetKey == NULL)
			{
				break;
			}
			
			sigOffset = GetOffset(sigOffsetKey, true);
			sigOffsetByte = symbols.GetKeyValue((const char *)sigByteName);
			
			if(sigOffset != -1 && sigOffsetByte != NULL)//Got the offset in the function
			{
				uint8_t iByte = strtoul(sigOffsetByte, NULL, 16);
				uint8_t iCompare = *(uint8_t *)((intptr_t)ptr + sigOffset);

				if(iByte == iCompare)
				{
					printf("     w: %s -> %s (%4d) \\x%02X == \\x%02X GOOD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
				}
				else
				{
					printf("!    w: %s -> %s (%4d) \\x%02X != \\x%02X BAD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
				}
			}
		}
	}
	else
	{
		sigOffset = GetOffset(sigOffsetKey, true);
		sigOffsetByte = symbols.GetKeyValue((const char *)sigByteName);
			
		if(sigOffset != -1 && sigOffsetByte != NULL)//Got the offset in the function
		{
			uint8_t iByte = strtoul(sigOffsetByte, NULL, 16);
			uint8_t iCompare = *(uint8_t *)((intptr_t)ptr + sigOffset);

			if(iByte == iCompare)
			{
				printf("     w: %s -> %s (%4d) \\x%02X == \\x%02X GOOD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
			}
			else
			{
				printf("!    w: %s -> %s (%4d) \\x%02X != \\x%02X BAD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
			}
		}
	}
}
void CheckLinuxSigOffset(char* name, const char* symbol, void * handle)
{
	void *ptr = GetLinuxSigPtr(handle, symbol);
	if(!ptr)
	{
		return;
	}
	const char* sigOffsetKey = NULL;
	const char* sigOffsetByte = NULL;
	int sigOffset = -1;
	char sigOffsetName[128];
	char sigByteName[128];

	snprintf(sigOffsetName, sizeof(sigOffsetName), "%s_Offset", name);
	snprintf(sigByteName, sizeof(sigByteName), "%s_Byte_Lin", name);

	sigOffsetKey = symbols.GetKeyValue((const char *)sigOffsetName);

	if(sigOffsetKey == NULL)
	{

		//Maybe it has multiple?
		for(unsigned int i = 1; i <= 4; i++)
		{
			snprintf(sigOffsetName, sizeof(sigOffsetName), "%s_Offset%i", name, i);
			snprintf(sigByteName, sizeof(sigByteName), "%s_Byte_Lin%i", name, i);
			sigOffsetKey = symbols.GetKeyValue((const char *)sigOffsetName);

			if(sigOffsetKey == NULL)
			{
				break;
			}
			
			sigOffset = GetOffset(sigOffsetKey, false);
			sigOffsetByte = symbols.GetKeyValue((const char *)sigByteName);
			
			if(sigOffset != -1 && sigOffsetByte != NULL)//Got the offset in the function
			{
				uint8_t iByte = strtoul(sigOffsetByte, NULL, 16);
				uint8_t iCompare = *(uint8_t *)((intptr_t)ptr + sigOffset);

				if(iByte == iCompare)
				{
					printf("     l: %s -> %s (%4d) \\x%02X == \\x%02X GOOD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
				}
				else
				{
					printf("!    l: %s -> %s (%4d) \\x%02X != \\x%02X BAD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
				}
			}
		}
	}
	else
	{
		sigOffset = GetOffset(sigOffsetKey, false);
		sigOffsetByte = symbols.GetKeyValue((const char *)sigByteName);
			
		if(sigOffset != -1 && sigOffsetByte != NULL)//Got the offset in the function
		{
			uint8_t iByte = strtoul(sigOffsetByte, NULL, 16);
			uint8_t iCompare = *(uint8_t *)((intptr_t)ptr + sigOffset);

			if(iByte == iCompare)
			{
				printf("     l: %s -> %s (%4d) \\x%02X == \\x%02X GOOD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
			}
			else
			{
				printf("!    l: %s -> %s (%4d) \\x%02X != \\x%02X BAD\n", name, sigOffsetKey, sigOffset, iCompare, iByte);
			}
		}
	}
}
int GetOffset(const char* key, bool windows)
{
	for (list<Offset>::iterator it = gc.m_Offsets.begin(); it != gc.m_Offsets.end(); it++)
	{
		if (strcmp(it->name, key) == 0)
		{
			if(windows)
				return it->win;
			else
				return it->lin;
		}
	}
	return -1;
}
void *GetWindowsSigPtr(int file, const char* symbol)
{
	int matches = 0;
	bool atFuncStart = true;
	bool isAt = (symbol[0] == '@');
	// we can't support this on windows from here
	if (isAt)
		return NULL;
	
	unsigned char real_sig[511];
	size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

	if (real_bytes >= 1)
	{
		return mu.FindPatternInFile(file, (char*)real_sig, real_bytes, matches, atFuncStart);
	}
	return NULL;

}
void *GetLinuxSigPtr(void *handle, const char* symbol)
{
	bool isAt = (symbol[0] == '@' && symbol[1] != '\0');
	int matches = 0;
	bool dummy;
	
	if (isAt)
	{
		if( use_symtab && mu.ResolveSymbol(handle, &symbol[1]) )
			return mu.ResolveSymbol(handle, &symbol[1]);
		else if( !use_symtab && dlsym(handle, &symbol[1]) )
			return dlsym(handle, &symbol[1]);
	}
	else
	{
		unsigned char real_sig[511];
		size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

		if (real_bytes >= 1)
		{
			struct link_map *dlmap = (struct link_map *)handle;
			
			return mu.FindPattern((void *)dlmap->l_addr, (char*)real_sig, real_bytes, matches, dummy);
		}
	}

	return NULL;
}
int checkSigStringW(int file, const char* symbol)
{
	int matches = 0;
	bool atFuncStart = true;
	bool isAt = (symbol[0] == '@');
	// we can't support this on windows from here
	if (isAt)
		return false;
	
	unsigned char real_sig[511];
	size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

	if (real_bytes >= 1)
	{
		mu.FindPatternInFile(file, (char*)real_sig, real_bytes, matches, atFuncStart);
	}

	if( !atFuncStart )
		return -1;
	
	return matches;
}

int checkSigStringL(void *handle, const char* symbol)
{
	bool isAt = (symbol[0] == '@' && symbol[1] != '\0');
	int matches = 0;
	bool dummy;
	
	if (isAt)
	{
		if( use_symtab && mu.ResolveSymbol(handle, &symbol[1]) )
			matches = 1;
		else if( !use_symtab && dlsym(handle, &symbol[1]) )
			matches = 1;
	}
	else
	{
		unsigned char real_sig[511];
		size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

		if (real_bytes >= 1)
		{
			// The pointer returned by dlopen is not inside the loaded librarys memory region.
			struct link_map *dlmap = (struct link_map *)handle;
			
			mu.FindPattern((void *)dlmap->l_addr, (char*)real_sig, real_bytes, matches, dummy);
		}
	}

	return matches;
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
	if( !use_symtab )
	{
		for (int i = 0; i < 1000; i++ )
		{
			void *pFunc = vt[i];
			if( !pFunc )
				continue;
			
			if( pFunc == dlsym(handle, symbol ) )
				return i - 2;
		}
		
		return -1;
	}
	
	int funcPos, paramPos;
	findFuncPos(symbol, funcPos, paramPos);

	for (int i = 0; i < 1000; i++)
	{
		void *pFunc = vt[i];
		if( !pFunc )
			continue;
		
		const char *s;

		if (!(s = mu.ResolveAddr(handle, pFunc)))
			continue;

		if ((i > 1) && (strncmp(s, "_ZTI", 4) == 0))
			break;


		int tempFuncPos, tempParamPos;
		findFuncPos(s, tempFuncPos, tempParamPos);

		if (strcmp(s + tempFuncPos, symbol + funcPos) == 0)
		{
			return i - 2;
		}
	}

	return -1;
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

size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[written - 1] = r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

