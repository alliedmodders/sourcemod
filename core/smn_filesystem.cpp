#include <sys/stat.h>
#include "sm_globals.h"
#include "HandleSys.h"
#include "LibrarySys.h"
#include "sm_stringutil.h"

HandleType_t g_FileType;
HandleType_t g_DirType;

class FileNatives : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	virtual void OnSourceModAllInitialized()
	{
		g_FileType = g_HandleSys.CreateType("File", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_DirType = g_HandleSys.CreateType("Directory", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	virtual void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_DirType, g_pCoreIdent);
		g_HandleSys.RemoveType(g_FileType, g_pCoreIdent);
		g_DirType = 0;
		g_FileType = 0;
	}
	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == g_FileType)
		{
			FILE *fp = (FILE *)object;
			fclose(fp);
		} else if (type == g_DirType) {
			IDirectory *pDir = (IDirectory *)object;
			g_LibSys.CloseDirectory(pDir);
		}
	}
};


static cell_t sm_OpenDirectory(IPluginContext *pContext, const cell_t *params)
{
	char *path;
	int err;
	if ((err=pContext->LocalToString(params[1], &path)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), path);

	IDirectory *pDir = g_LibSys.OpenDirectory(realpath);
	if (!pDir)
	{
		return 0;
	}

	return g_HandleSys.CreateHandle(g_DirType, pDir, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t sm_ReadDirEntry(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	IDirectory *pDir;
	HandleError herr;
	HandleSecurity sec;
	int err;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_DirType, &sec, (void **)&pDir))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	if (!pDir->MoreFiles())
	{
		return false;
	}

	cell_t *filetype;
	if ((err=pContext->LocalToPhysAddr(params[4], &filetype)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	if (pDir->IsEntryDirectory())
	{
		*filetype = 1;
	} else if (pDir->IsEntryFile()) {
		*filetype = 2;
	} else {
		*filetype = 0;
	}

	const char *path = pDir->GetEntryName();
	if ((err=pContext->StringToLocalUTF8(params[2], params[3], path, NULL))
		!= SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	pDir->NextEntry();

	return true;
}

static cell_t sm_OpenFile(IPluginContext *pContext, const cell_t *params)
{
	char *name, *mode;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}
	if ((err=pContext->LocalToString(params[2], &mode)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);

	FILE *pFile = fopen(realpath, mode);
	if (!pFile)
	{
		return 0;
	}

	return g_HandleSys.CreateHandle(g_FileType, pFile, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t sm_DeleteFile(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);

	return (unlink(realpath)) ? 0 : 1;
}

static cell_t sm_ReadFileLine(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;
	int err;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	char *buf;
	if ((err=pContext->LocalToString(params[2], &buf)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	fgets(buf, params[3], pFile);

	return 1;
}

static cell_t sm_IsEndOfFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return (feof(pFile)) ? 1 : 0;
}

static cell_t sm_FileSeek(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	fseek(pFile, params[2], params[3]);
	
	return 1;
}

static cell_t sm_FilePosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return ftell(pFile);
}

static cell_t sm_FileExists(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(realpath, &s) != 0)
	{
		return 0;
	}
	if (s.st_mode & S_IFREG)
	{
		return 1;
	}
	return 0;
#else if defined PLATFORM_POSIX
	struct stat s;
	if (stat(realpath, &s) != 0)
	{
		return 0;
	}
	if (S_ISREG(s.st_mode))
	{
		return 1;
	}
	return 0;
#endif
}

static cell_t sm_RenameFile(IPluginContext *pContext, const cell_t *params)
{
	char *newpath, *oldpath;
	int err;
	if ((err=pContext->LocalToString(params[1], &newpath)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}
	if ((err=pContext->LocalToString(params[2], &oldpath)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char new_realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(new_realpath, sizeof(new_realpath), "%s/%s", g_SourceMod.GetBaseDir(), newpath);
	char old_realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(old_realpath, sizeof(old_realpath), "%s/%s", g_SourceMod.GetBaseDir(), oldpath);

#ifdef PLATFORM_WINDOWS
	return (MoveFileA(old_realpath, new_realpath)) ? 1 : 0;
#else if defined PLATFORM_POSIX
	return (rename(old_realpath, new_realpath)) ? 0 : 1;
#endif
}

static cell_t sm_DirExists(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(realpath, &s) != 0)
	{
		return 0;
	}
	if (s.st_mode & S_IFDIR)
	{
		return 1;
	}
	return 0;
#else if defined PLATFORM_POSIX
	struct stat s;
	if (stat(realpath, &s) != 0)
	{
		return 0;
	}
	if (S_ISDIR(s.st_mode))
	{
		return 1;
	}
	return 0;
#endif
}

static cell_t sm_FileSize(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return -1;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(realpath, &s) != 0)
	{
		return -1;
	}
	if (s.st_mode & S_IFREG)
	{
		return static_cast<cell_t>(s.st_size);
	}
	return -1;
#else if defined PLATFORM_POSIX
	struct stat s;
	if (stat(realpath, &s) != 0)
	{
		return -1;
	}
	if (S_ISREG(s.st_mode))
	{
		return static_cast<cell_t>(s.st_size);
	}
	return -1;
#endif
}

static cell_t sm_RemoveDir(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char realpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(realpath, sizeof(realpath), "%s/%s", g_SourceMod.GetBaseDir(), name);

	return (rmdir(realpath)) ? 0 : 1;
}

static cell_t sm_WriteFileLine(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	char *fmt;
	int err;
	if ((err=pContext->LocalToString(params[2], &fmt)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char buffer[2048];
	int arg = 3;
	atcprintf(buffer, sizeof(buffer), fmt, pContext, params, &arg);
	fprintf(pFile, "%s\n", buffer);

	return 1;
}

static FileNatives s_FileNatives;

REGISTER_NATIVES(filesystem)
{
	{"OpenDirectory",			sm_OpenDirectory},
	{"ReadDirEntry",			sm_ReadDirEntry},
	{"OpenFile",				sm_OpenFile},
	{"DeleteFile",				sm_DeleteFile},
	{"ReadFileLine",			sm_ReadFileLine},
	{"IsEndOfFile",				sm_IsEndOfFile},
	{"FileSeek",				sm_FileSeek},
	{"FilePosition",			sm_FilePosition},
	{"FileExists",				sm_FileExists},
	{"RenameFile",				sm_RenameFile},
	{"DirExists",				sm_DirExists},
	{"FileSize",				sm_FileSize},
	{"RemoveDir",				sm_RemoveDir},
	{"WriteFileLine",			sm_WriteFileLine},
	{NULL,						NULL},
};
