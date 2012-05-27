#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <math.h>
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

int main(int argc, char **argv)
{
	char *gamedata = NULL;
	bool debug = false;

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "b:de:f:g:x:w:y:")) != -1)
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

			case '?':
				printf("Usage: ./gdc -g <game> -e <engine name> -f <gamedata file> -b <game binary> -x <engine binary> -w <win game binary> -y <win engine binary>\n");
				return 0;

			default:
				printf("WHAT\n");
				return 0;
		}

	if (!game || !engine || !gamedata || !game_binary || !engine_binary || !wgame_binary || !wengine_binary)
	{
		printf("Usage: ./gdc -g <game> -e <engine name> -f <gamedata file> -b <game binary> -x <engine binary> -w <win game binary> -y <win engine binary>\n");
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

	const char *vtsym = symbols.GetKeyValue("vtsym");
	if (!vtsym)
	{
		printf("Couldn't find vtsym\n");
		return 0;
	}

	void **vt = (void**) mu.ResolveSymbol(ghandle, vtsym);
	if (!vt)
	{
		printf("Couldn't find vtable %s\n", vtsym);
		dlclose(ghandle);
		dlclose(ehandle);
		close(wgfile);
		close(wefile);

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
			int newOffset;
			char symvt[128];
			strcpy(symvt, it->name);
			strcat(symvt, "_vt");
			const char *newvtsym = symbols.GetKeyValue(symvt);
			if (newvtsym && newvtsym[0])
			{
				void **newvt = (void**) mu.ResolveSymbol(ghandle, newvtsym);
			        if (!newvt)
				{
					printf("O: %-22s - can't find, skipping\n", symvt);
					continue;
				}

				newOffset = findVFunc(ghandle, newvt, symbol);
			}
			else
			{
				newOffset = findVFunc(ghandle, vt, symbol);
			}

			if (newOffset == it->lin)
			{
				printf("O: %-22s - GOOD.    current [ w: %3d, l: %3d ].\n", it->name, it->win, it->lin);
			}
			else
			{
				printf("O: %-22s - CHANGED. old [ w: %3d, l: %3d ]. new [ w: %3d, l: %3d ].\n",
					it->name,
					it->win, it->lin,
					newOffset - (it->lin - it->win), newOffset
					);
			}
		}
		else // !symbol
		{
			printf("O: %-22s - no Linux symbol, skipping\n", it->name);
		}
	}

	printf("\nWindows offsets are (semi-)wild guesses!\n\n");

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
			printf("S: %-22s - hasn't linux nor windows data, skipping\n", it->name);
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
			printf("S: %-22s - isn't from server nor engine, skipping\n", it->name);
			continue;
		}
		
		bool foundLinux   = (!hasLinux   || checkSigStringL(linHandle, linSymbol));
		bool foundWindows = (!hasWindows || checkSigStringW(winFile,   winSymbol));
			
		// Prepare for ternery ABUSE
		
		if (foundLinux && foundWindows)
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
			printf("S: %-22s (%s) - GOOD.\n",
				it->name,
				(it->lib == Server) ? "server" : "engine"
				);
		}
		else
		{
			// extra \n at end is intentional to add buffer after possibly long sigs
			printf("S: %-22s (%s) - w: %s - l: %s\n%s%s%s%s%s%s%s%s\n",
					it->name,
					(it->lib == Server) ? "server" : "engine",
					hasWindows ? (foundWindows ? "GOOD" : "CHANGED") : "UNKNOWN",
					hasLinux   ? (foundLinux   ? "GOOD" : "CHANGED") : "UNKNOWN",
					
					hasWindows ? "\tcurrent - w: " : "",
					(hasWindows && foundWindows) ? "    (found) \"" : (hasWindows ? "(NOT found) \"" : ""),
					(hasWindows && winSymbol)  ? winSymbol : "",
					hasWindows ? "\"\n" : "",
					
					hasLinux ? "\tcurrent - l: " : "",
					(hasLinux && foundLinux) ? "    (found) \"" : (hasLinux ? "(NOT found) \"" : ""),
					(hasLinux && linSymbol)  ? linSymbol : "",
					hasLinux ? "\"\n" : ""
					);
			
		}
		
		//  j/k, no way that anyone could have been prepared for that
	}

	return 0;
}

bool checkSigStringW(int file, const char* symbol)
{
	void *addr = NULL;
	bool isAt = (symbol[0] == '@');
	// we can't support this on windows from here
	if (isAt)
		return false;
	
	unsigned char real_sig[511];
	size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

	if (real_bytes >= 1)
	{
		addr = mu.FindPatternInFile(file, (char*)real_sig, real_bytes);
	}

	if (addr)
		return true;
	
	return false;
}

bool checkSigStringL(void *handle, const char* symbol)
{
	void *addr = NULL;
	bool isAt = (symbol[0] == '@');
	
	if (isAt)
	{
		addr = mu.ResolveSymbol(handle, symbol + 1);
	}
	else
	{
		unsigned char real_sig[511];
		size_t real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), symbol);

		if (real_bytes >= 1)
		{
			addr = mu.FindPattern(handle, (char*)real_sig, real_bytes);
		}
	}

	if (addr)
		return true;
	
	return false;
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
	int funcPos, paramPos;
	findFuncPos(symbol, funcPos, paramPos);



	for (int i = 0; i < 1000; i++)
	{
		void *pFunc = vt[i];
		const char *s;

		if (!(s = mu.ResolveAddr(handle, pFunc)))
			continue;

		if ((i > 1) && (strncmp(s, "_ZTI", 4) == 0))
			break;


		int tempFuncPos, tempParamPos;
		findFuncPos(s, tempFuncPos, tempParamPos);

		if (strcmp(s + tempFuncPos, symbol + funcPos) == 0)
		{
			offset = i;
			break;
		}
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

