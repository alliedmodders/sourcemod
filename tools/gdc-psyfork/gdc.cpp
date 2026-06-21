#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <link.h>
#include <cxxabi.h>
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
bool use_color = false;
CGameConfig symbols;
CGameConfig *gc_ptr = NULL;

#define CR   (use_color ? "\033[0m"  : "")
#define CGRN (use_color ? "\033[32m" : "")
#define CRED (use_color ? "\033[31m" : "")
#define CYLW (use_color ? "\033[33m" : "")
#define CBLD (use_color ? "\033[1m"  : "")

enum OffsetStatus { OFF_GOOD, OFF_CHANGED, OFF_NOSYM, OFF_NOVTABLE };

struct OffsetResult
{
	const char *name;
	char vtable[128];
	OffsetStatus status;
	int lin, win, newLin, newWin;
};

struct SigResult
{
	const char *name;
	bool isServer;
	char winStatus[16];
	char linStatus[16];
	const char *winSym;
	const char *linSym;
	bool bWinGood, bLinGood;
	bool showWinExtra, showLinExtra;
};

// Returns a human-readable name for a mangled symbol, or the symbol itself if
// it can't be demangled. The result is leaked, but gdc is a one-shot CLI.
static const char *Demangle(const char *sym)
{
	if (!sym || !sym[0])
		return sym;

	const char *s = (sym[0] == '@') ? sym + 1 : sym;
	int status;
	char *d = abi::__cxa_demangle(s, NULL, NULL, &status);
	return (status == 0 && d) ? d : sym;
}

inline bool IsDigit( char c )
{
	return c < 58 && c > 47;
}

void PrintUsage()
{
	printf("Usage: ./gdc -g <game> -e <engine name> -f <gamedata file> [-f <gamedata file> ...] -b <game binary> -x <engine binary> -w <win game binary> -y <win engine binary> [-s <symbols.txt file>]\n");
	printf("Other options:\n");
	printf(" -d\tAdd debug output\n");
	printf(" -n\tDon't use symbol table for lookups (falls back to dlsym)\n");
}

static void ProcessFile(const char *gamedata, bool debug,
	void *ghandle, void *ehandle, int wgfile, int wefile,
	int &out_off_good, int &out_off_changed, int &out_off_skipped,
	int &out_sig_good, int &out_sig_issues)
{
	CGameConfig gc_local;
	gc_ptr = &gc_local;

	char err[512];
	if (!gc_ptr->EnterFile(gamedata, err, sizeof(err)))
	{
		printf("Error loading %s: %s\n", gamedata, err);
		out_off_good = out_off_changed = out_off_skipped = 0;
		out_sig_good = out_sig_issues = 0;
		return;
	}

	// Collect offset results
	std::vector<OffsetResult> offsets;
	for (list<Offset>::iterator it = gc_ptr->m_Offsets.begin(); it != gc_ptr->m_Offsets.end(); it++)
	{
		if (debug)
			printf("DEBUG %s - l: %d w: %d\n", it->name, it->lin, it->win);

		OffsetResult r;
		r.name = it->name;
		r.vtable[0] = 0;
		r.lin = it->lin;
		r.win = it->win;
		r.newLin = it->lin;
		r.newWin = it->win;

		const char *symbol = symbols.GetKeyValue(it->name);
		if (!symbol)
		{
			r.status = OFF_NOSYM;
			offsets.push_back(r);
			continue;
		}

		char symvt[128] = "_ZTV";
		bool bGotFirstNumbers = false;
		bool bLastNumberDigit = false;
		unsigned int symlen = strlen(symbol);
		unsigned int pos = strlen(symvt);

		for ( unsigned int i = 0; i < symlen && pos < (sizeof(symvt)-1); ++i )
		{
			bool isDigit = IsDigit( symbol[i] );
			if( !isDigit && bLastNumberDigit )
				bGotFirstNumbers = true;
			if( !bGotFirstNumbers && !isDigit )
				continue;
			if( bGotFirstNumbers && isDigit )
			{
				symvt[pos] = 0;
				break;
			}
			symvt[pos++] = symbol[i];
			if( isDigit )
				bLastNumberDigit = isDigit;
		}
		symvt[pos] = 0;

		if (!symvt[0])
		{
			r.status = OFF_NOSYM;
			offsets.push_back(r);
			continue;
		}

		void **newvt = NULL;
		if( use_symtab )
			newvt = (void**)mu.ResolveSymbol(ghandle, symvt);
		else
			newvt = (void **)dlsym(ghandle, symvt);

		if (!newvt)
		{
			r.status = OFF_NOVTABLE;
			strncopy(r.vtable, symvt, sizeof(r.vtable));
			offsets.push_back(r);
			continue;
		}

		int newOffset = findVFunc(ghandle, newvt, symbol);
		if (newOffset == it->lin)
		{
			r.status = OFF_GOOD;
		}
		else
		{
			r.status = OFF_CHANGED;
			r.newLin = newOffset;
			r.newWin = newOffset - (it->lin - it->win);
		}
		offsets.push_back(r);
	}

	// Collect sig results
	std::vector<SigResult> sigs;
	for (list<Sig>::iterator it = gc_ptr->m_Sigs.begin(); it != gc_ptr->m_Sigs.end(); it++)
	{
		if (debug)
			printf("DEBUG %s - %s - l: %s\n", it->name, it->lib == Server ? "server" : "engine", it->lin);

		const char *linSymbol = it->lin;
		const char *winSymbol = it->win;
		bool hasLinux   = (linSymbol && linSymbol[0]);
		bool hasWindows = (winSymbol && winSymbol[0]);

		if (!hasLinux && !hasWindows)
			continue;

		if (it->lib != Server && it->lib != Engine)
			continue;

		int  winFile    = (it->lib == Server) ? wgfile : wefile;
		void *linHandle = (it->lib == Server) ? ghandle : ehandle;

		int linuxMatches = 0, windowsMatches = 0;
		if( hasLinux )   linuxMatches   = checkSigStringL(linHandle, linSymbol);
		if( hasWindows ) windowsMatches = checkSigStringW(winFile, winSymbol);

		SigResult r;
		r.name     = it->name;
		r.isServer = (it->lib == Server);
		r.winSym   = winSymbol;
		r.linSym   = linSymbol;

		const char *options = symbols.GetOptionValue(it->name);
		bool allowMulti   = ( options && strstr(options, "allowmultiple") != NULL );
		bool allowMidfunc = ( options && strstr(options, "allowmidfunc")  != NULL );

		r.bWinGood = false;
		if( !hasWindows )
			snprintf( r.winStatus, sizeof(r.winStatus), "UNKNOWN" );
		else if( windowsMatches == -1 && !allowMidfunc )
			snprintf( r.winStatus, sizeof(r.winStatus), "MIDFUNC" );
		else if( windowsMatches == 0 )
			snprintf( r.winStatus, sizeof(r.winStatus), "NOTFOUND" );
		else if( windowsMatches > 1 && !allowMulti )
			snprintf( r.winStatus, sizeof(r.winStatus), "MULTIPLE" );
		else
		{
			r.bWinGood = true;
			snprintf( r.winStatus, sizeof(r.winStatus), "GOOD" );
		}

		r.bLinGood = false;
		if( !hasLinux )
			snprintf( r.linStatus, sizeof(r.linStatus), "UNKNOWN" );
		else if( linuxMatches == 0 )
			snprintf( r.linStatus, sizeof(r.linStatus), "NOTFOUND" );
		else if( linuxMatches == 1 )
		{
			r.bLinGood = true;
			snprintf( r.linStatus, sizeof(r.linStatus), "GOOD" );
		}
		else
			snprintf( r.linStatus, sizeof(r.linStatus), "CHECK" );

		r.showWinExtra = (hasWindows && !r.bWinGood);
		r.showLinExtra = ( (hasLinux && !r.bLinGood) || r.showWinExtra );

		sigs.push_back(r);
	}

	// Count
	int off_good = 0, off_changed = 0, off_skipped = 0;
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		if      (offsets[i].status == OFF_GOOD)    off_good++;
		else if (offsets[i].status == OFF_CHANGED) off_changed++;
		else                                       off_skipped++;
	}
	int sig_good = 0, sig_issues = 0;
	for (unsigned int i = 0; i < sigs.size(); i++)
	{
		if (sigs[i].bWinGood && sigs[i].bLinGood)
			sig_good++;
		else
			sig_issues++;
	}

	out_off_good    = off_good;
	out_off_changed = off_changed;
	out_off_skipped = off_skipped;
	out_sig_good    = sig_good;
	out_sig_issues  = sig_issues;

	// Header + summary
	printf("Gamedata: %s\n", gamedata);
	printf("Offsets: %s%d good%s  %s%d changed%s  %d skipped  |  Sigs: %s%d good%s  %s%d issues%s\n",
		off_good    ? CGRN : "", off_good,    off_good    ? CR : "",
		off_changed ? CRED : "", off_changed, off_changed ? CR : "",
		off_skipped,
		sig_good   ? CGRN : "", sig_good,   sig_good   ? CR : "",
		sig_issues ? CRED : "", sig_issues, sig_issues ? CR : "");

	// If every changed offset shifted by the same amount, it's likely a single
	// vtable insertion/removal rather than many independent changes.
	if (off_changed >= 2)
	{
		int first_delta = 0;
		bool uniform = true, first = true;
		for (unsigned int i = 0; i < offsets.size(); i++)
		{
			if (offsets[i].status != OFF_CHANGED)
				continue;

			int delta = offsets[i].newLin - offsets[i].lin;
			if (first)
			{
				first_delta = delta;
				first = false;
			}
			else if (delta != first_delta)
			{
				uniform = false;
				break;
			}
		}
		if (uniform)
			printf("  %s(all %d changed offsets shifted by %+d, likely a vtable %s)%s\n",
				CYLW, off_changed, first_delta,
				first_delta > 0 ? "insertion" : "removal", CR);
	}

	// Problems first, so they stand out before the full listing.
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		if (offsets[i].status == OFF_CHANGED)
			printf("%s! O: %-26s CHANGED%s  old [ w:%3d  l:%3d ]  new [ w:%3d  l:%3d ]\n",
				CRED, offsets[i].name, CR,
				offsets[i].win, offsets[i].lin,
				offsets[i].newWin, offsets[i].newLin);
	}
	for (unsigned int i = 0; i < sigs.size(); i++)
	{
		if (sigs[i].bWinGood && sigs[i].bLinGood)
			continue;

		const char *wcolor = sigs[i].bWinGood ? CGRN : CRED;
		const char *lcolor = sigs[i].bLinGood ? CGRN : CRED;
		printf("%s! S: %-26s%s (%s)  w: %s%-8s%s  l: %s%-8s%s\n",
			CRED, sigs[i].name, CR,
			sigs[i].isServer ? "server" : "engine",
			wcolor, sigs[i].winStatus, CR,
			lcolor, sigs[i].linStatus, CR);
		if (!sigs[i].bLinGood && sigs[i].linSym && sigs[i].linSym[0] == '@')
			printf("     -> %s\n", Demangle(sigs[i].linSym));
		else if (!sigs[i].bWinGood && sigs[i].winSym && sigs[i].winSym[0] == '@')
			printf("     -> %s\n", Demangle(sigs[i].winSym));
	}

	// Full listing
	printf("%s=== Offsets ===%s\n", CBLD, CR);
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		const OffsetResult &r = offsets[i];
		switch (r.status)
		{
			case OFF_GOOD:
				printf("  O: %-26s %sGOOD%s    [ w:%3d  l:%3d ]\n",
					r.name, CGRN, CR, r.win, r.lin);
				break;
			case OFF_CHANGED:
				printf("%s! O: %-26s CHANGED%s  old [ w:%3d  l:%3d ]  new [ w:%3d  l:%3d ]\n",
					CRED, r.name, CR, r.win, r.lin, r.newWin, r.newLin);
				break;
			case OFF_NOSYM:
				printf("  O: %-26s no Linux symbol\n", r.name);
				break;
			case OFF_NOVTABLE:
				printf("%s? O: %-26s%s %s not found\n", CYLW, r.name, CR, Demangle(r.vtable));
				break;
		}
	}

	printf("%s=== Signatures ===%s\n", CBLD, CR);
	printf("  (Windows offsets are estimates; sig-internal offsets are guesses)\n");
	for (unsigned int i = 0; i < sigs.size(); i++)
	{
		const SigResult &r = sigs[i];
		const char *lib = r.isServer ? "server" : "engine";
		if (r.bWinGood && r.bLinGood)
		{
			printf("  S: %-26s (%s)  w: %sGOOD%s      l: %sGOOD%s\n",
				r.name, lib, CGRN, CR, CGRN, CR);

			// Check whether the signature has a matching offset
			CheckWindowsSigOffset((char*)r.name, r.winSym, r.isServer ? wgfile : wefile);
			CheckLinuxSigOffset((char*)r.name, r.linSym, r.isServer ? ghandle : ehandle);
		}
		else
		{
			const char *wcolor = r.bWinGood ? CGRN : CRED;
			const char *lcolor = r.bLinGood ? CGRN : CRED;
			printf("%s! S: %-26s%s (%s)  w: %s%-8s%s  l: %s%-8s%s\n",
				CRED, r.name, CR, lib,
				wcolor, r.winStatus, CR,
				lcolor, r.linStatus, CR);
			if (r.showWinExtra)
			{
				printf("     w: \"%s\"\n", r.winSym ? r.winSym : "");
				if (r.winSym && r.winSym[0] == '@')
					printf("     -> %s\n", Demangle(r.winSym));
			}
			if (r.showLinExtra)
			{
				printf("     l: \"%s\"\n", r.linSym ? r.linSym : "");
				if (r.linSym && r.linSym[0] == '@')
					printf("     -> %s\n", Demangle(r.linSym));
			}
		}
	}
}

int main(int argc, char **argv)
{
	std::vector<const char *> gamedata_files;
	bool debug = false;

	use_color = isatty(STDOUT_FILENO);

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
				gamedata_files.push_back(optarg);
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

	if (!game || !engine || gamedata_files.empty() || !game_binary || !engine_binary || !wgame_binary || !wengine_binary)
	{
		PrintUsage();
		return 0;
	}

	void *ghandle = dlopen(game_binary, RTLD_LAZY);
	if (!ghandle)
	{
		printf("Couldn't open %s (%s)\n", game_binary, dlerror());
		return 0;
	}
	void *ehandle = dlopen(engine_binary, RTLD_LAZY);
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
	if (!symbols.EnterFile(symbols_file ? symbols_file : "symbols.txt", err, sizeof(err)))
	{
		printf("symbols.txt: %s\n", err);
		return 0;
	}

	printf("Game: %s\n", game);
	if (debug)
		printf("Engine: %s  Game binary: %s  Engine binary: %s\n", engine, game_binary, engine_binary);

	int total_off_good = 0, total_off_changed = 0, total_off_skipped = 0;
	int total_sig_good = 0, total_sig_issues = 0;

	for (unsigned int fi = 0; fi < gamedata_files.size(); fi++)
	{
		if (fi > 0)
			printf("---\n");

		int og, oc, osk, sg, si;
		ProcessFile(gamedata_files[fi], debug,
			ghandle, ehandle, wgfile, wefile,
			og, oc, osk, sg, si);

		total_off_good    += og;
		total_off_changed += oc;
		total_off_skipped += osk;
		total_sig_good    += sg;
		total_sig_issues  += si;
	}

	if (gamedata_files.size() > 1)
	{
		printf("---\n");
		printf("Total (%d files)  Offsets: %s%d good%s  %s%d changed%s  %d skipped  |  Sigs: %s%d good%s  %s%d issues%s\n",
			(int)gamedata_files.size(),
			total_off_good    ? CGRN : "", total_off_good,    total_off_good    ? CR : "",
			total_off_changed ? CRED : "", total_off_changed, total_off_changed ? CR : "",
			total_off_skipped,
			total_sig_good   ? CGRN : "", total_sig_good,   total_sig_good   ? CR : "",
			total_sig_issues ? CRED : "", total_sig_issues, total_sig_issues ? CR : "");
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
					printf("     w: %s -> %s (%4d) \\x%02X == \\x%02X %sGOOD%s\n", name, sigOffsetKey, sigOffset, iCompare, iByte, CGRN, CR);
				}
				else
				{
					printf("%s!    w: %s -> %s (%4d) \\x%02X != \\x%02X BAD%s\n", CRED, name, sigOffsetKey, sigOffset, iCompare, iByte, CR);
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
				printf("     w: %s -> %s (%4d) \\x%02X == \\x%02X %sGOOD%s\n", name, sigOffsetKey, sigOffset, iCompare, iByte, CGRN, CR);
			}
			else
			{
				printf("%s!    w: %s -> %s (%4d) \\x%02X != \\x%02X BAD%s\n", CRED, name, sigOffsetKey, sigOffset, iCompare, iByte, CR);
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
					printf("     l: %s -> %s (%4d) \\x%02X == \\x%02X %sGOOD%s\n", name, sigOffsetKey, sigOffset, iCompare, iByte, CGRN, CR);
				}
				else
				{
					printf("%s!    l: %s -> %s (%4d) \\x%02X != \\x%02X BAD%s\n", CRED, name, sigOffsetKey, sigOffset, iCompare, iByte, CR);
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
				printf("     l: %s -> %s (%4d) \\x%02X == \\x%02X %sGOOD%s\n", name, sigOffsetKey, sigOffset, iCompare, iByte, CGRN, CR);
			}
			else
			{
				printf("%s!    l: %s -> %s (%4d) \\x%02X != \\x%02X BAD%s\n", CRED, name, sigOffsetKey, sigOffset, iCompare, iByte, CR);
			}
		}
	}
}
int GetOffset(const char* key, bool windows)
{
	for (list<Offset>::iterator it = gc_ptr->m_Offsets.begin(); it != gc_ptr->m_Offsets.end(); it++)
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

