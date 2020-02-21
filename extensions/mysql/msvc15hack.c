
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

// Fix from from https://stackoverflow.com/a/34655235.
//
// __iob_func required by the MySQL we use,
// but no longer exists in the VS 14.0+ crt.

#pragma comment(lib, "DbgHelp.lib")
#pragma warning(disable:4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <DbgHelp.h>
#include <corecrt_wstdio.h>

#define GET_CURRENT_CONTEXT(c, contextFlags) \
  do { \
	c.ContextFlags = contextFlags; \
	__asm    call x \
	__asm x: pop eax \
	__asm    mov c.Eip, eax \
	__asm    mov c.Ebp, ebp \
	__asm    mov c.Esp, esp \
  } while(0);


FILE * __cdecl __iob_func(void)
{
	CONTEXT c = { 0 };
	STACKFRAME64 s = { 0 };
	DWORD imageType;
	HANDLE hThread = GetCurrentThread();
	HANDLE hProcess = GetCurrentProcess();

	GET_CURRENT_CONTEXT(c, CONTEXT_FULL);

	imageType = IMAGE_FILE_MACHINE_I386;
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Esp;
	s.AddrStack.Mode = AddrModeFlat;

	if (!StackWalk64(imageType, hProcess, hThread, &s, &c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
	{
		return NULL;
	}

	if (s.AddrReturn.Offset == 0)
	{
		return NULL;
	}

	{
		unsigned char const * assembly = (unsigned char const *)(s.AddrReturn.Offset);

		if (*assembly == 0x83 && *(assembly + 1) == 0xC0 && (*(assembly + 2) == 0x20 || *(assembly + 2) == 0x40))
		{
			if (*(assembly + 2) == 32)
			{
				return (FILE*)((unsigned char *)stdout - 32);
			}
			if (*(assembly + 2) == 64)
			{
				return (FILE*)((unsigned char *)stderr - 64);
			}

		}
		else
		{
			return stdin;
		}
	}

	return NULL;
}

// Adapted from dosmap.c in Visual Studio 12.0 CRT sources.
//
// The _dosmaperr function is required by the MySQL lib we use,
// but no longer exists in the VS 14.0+ crt.

static struct errentry
{
	DWORD oscode;      // OS return value
	int   errnocode;   // System V error code

} errtable[] =
{
	{ ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
	{ ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
	{ ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
	{ ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
	{ ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
	{ ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
	{ ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
	{ ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
	{ ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
	{ ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
	{ ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
	{ ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
	{ ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
	{ ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
	{ ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
	{ ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
	{ ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
	{ ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
	{ ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
	{ ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
	{ ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
	{ ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
	{ ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
	{ ERROR_FAIL_I24,               EACCES    },  /* 83 */
	{ ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
	{ ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
	{ ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
	{ ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
	{ ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
	{ ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
	{ ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
	{ ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
	{ ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
	{ ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
	{ ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
	{ ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
	{ ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
	{ ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
	{ ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
	{ ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
	{ ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
	{ ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
	{ ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
	{ ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
	{ ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }   /* 1816 */
};

// The following two constants must be the minimum and maximum
// values in the (contiguous) range of Exec Failure errors.
#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN

// These are the low and high value in the range of errors that are
// access violations
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED

void _dosmaperr(DWORD oserrno)
{
	_doserrno = oserrno;

	// Check the table for the OS error code
	for (size_t i = 0; i < _countof(errtable); ++i)
	{
		if (oserrno == errtable[i].oscode)
		{
			errno = errtable[i].errnocode;
		}
	}

	// The error code wasn't in the table.  We check for a range of
	// EACCES errors or exec failure errors (ENOEXEC).  Otherwise
	// EINVAL is returned.
	if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
		errno = EACCES;
	else if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
		errno = ENOEXEC;
	else
		errno = EINVAL;
}
