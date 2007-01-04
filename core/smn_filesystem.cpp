#include "sm_globals.h"
#include "HandleSys.h"
#include "LibrarySys.h"

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


cell_t sm_OpenDirectory(IPluginContext *pContext, const cell_t *params)
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

cell_t sm_ReadDirEntry(IPluginContext *pContext, const cell_t *params)
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

cell_t PrintStuff(IPluginContext *pContext, const cell_t *params)
{
	char *stuff;
	pContext->LocalToString(params[1], &stuff);

	FILE *fp = fopen("c:\\debug.txt", "at");
	fprintf(fp, "%s\n", stuff);
	fclose(fp);

	return 0;
}

static FileNatives s_FileNatives;

REGISTER_NATIVES(filesystem)
{
	{"OpenDirectory",			sm_OpenDirectory},
	{"ReadDirEntry",			sm_ReadDirEntry},
	{"PrintStuff",				PrintStuff},
	{NULL,						NULL},
};
