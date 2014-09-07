/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <sys/stat.h>
#include <string.h>
#include <IHandleSys.h>
#include <ILibrarySys.h>
#include <IPluginSys.h>
#include <IForwardSys.h>
#include <ISourceMod.h>
#include <ITranslator.h>
#include "common_logic.h"
#include "Logger.h"

#if defined PLATFORM_WINDOWS
#include <io.h>

#define FPERM_U_READ		0x0100	/* User can read. */
#define FPERM_U_WRITE		0x0080	/* User can write. */
#define FPERM_U_EXEC		0x0040	/* User can exec. */
#define FPERM_G_READ		0x0020	/* Group can read. */
#define FPERM_G_WRITE		0x0010	/* Group can write. */
#define FPERM_G_EXEC		0x0008	/* Group can exec. */
#define FPERM_O_READ		0x0004	/* Anyone can read. */
#define FPERM_O_WRITE		0x0002	/* Anyone can write. */
#define FPERM_O_EXEC		0x0001	/* Anyone can exec. */
#endif

HandleType_t g_FileType;
HandleType_t g_ValveFileType;
HandleType_t g_DirType;
HandleType_t g_ValveDirType;
IChangeableForward *g_pLogHook = NULL;

enum class FSType
{
	STDIO,
	VALVE,
};

class FSHelper
{
public:
	void SetFSType(FSType fstype)
	{
		_fstype = fstype;
	}
public:
	inline void *Open(const char *filename, const char *mode, const char *pathID)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->Open(filename, mode, pathID);
		else
			return fopen(filename, mode);
	}

	inline int Read(void *pOut, int size, void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->Read(pOut, size, (FileHandle_t)pFile);
		else
			return fread(pOut, 1, size, (FILE *)pFile) * size;
	}

	inline char *ReadLine(char *pOut, int size, void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->ReadLine(pOut, size, (FileHandle_t)pFile);
		else
			return fgets(pOut, size, (FILE *)pFile);
	}

	inline size_t Write(const void *pData, int size, void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return (size_t)smcore.filesystem->Write(pData, size, (FileHandle_t)pFile);
		else
			return fwrite(pData, 1, size, (FILE *)pFile);
	}

	inline void Seek(void *pFile, int pos, int seekType)
	{
		if (_fstype == FSType::VALVE)
			smcore.filesystem->Seek((FileHandle_t)pFile, pos, seekType);
		else
			fseek((FILE *)pFile, pos, seekType);
	}

	inline int Tell(void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->Tell((FileHandle_t)pFile);
		else
			return ftell((FILE *)pFile);
	}

	inline bool HasError(void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return !smcore.filesystem->IsOk((FileHandle_t)pFile);
		else
			return ferror((FILE *)pFile) != 0;
	}

	inline void Close(void *pFile)
	{
		if (_fstype == FSType::VALVE)
			smcore.filesystem->Close((FileHandle_t)pFile);
		else
			fclose((FILE *)pFile);
	}

	inline bool Remove(const char *pFilePath, const char *pathID)
	{
		if (_fstype == FSType::VALVE)
		{
			if (!smcore.filesystem->FileExists(pFilePath, pathID))
				return false;

			smcore.filesystem->RemoveFile(pFilePath, pathID);

			if (smcore.filesystem->FileExists(pFilePath, pathID))
				return false;

			return true;
		}
		else
		{
			return unlink(pFilePath) == 0;
		}
	}

	inline bool EndOfFile(void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->EndOfFile((FileHandle_t)pFile);
		else
			return feof((FILE *)pFile) != 0;
	}

	inline bool Flush(void *pFile)
	{
		if (_fstype == FSType::VALVE)
			return smcore.filesystem->Flush((FileHandle_t)pFile), true;
		else
			return fflush((FILE *)pFile) == 0;
	}
private:
	FSType _fstype;
};

struct ValveDirectory
{
	FileFindHandle_t hndl;
	char szFirstPath[PLATFORM_MAX_PATH];
	bool bHandledFirstPath;
};

class FileNatives : 
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener
{
public:
	FileNatives()
	{
	}
	virtual void OnSourceModAllInitialized()
	{
		g_FileType = handlesys->CreateType("File", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_ValveFileType = handlesys->CreateType("ValveFile", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_DirType = handlesys->CreateType("Directory", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_ValveDirType = handlesys->CreateType("ValveDirectory", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_pLogHook = forwardsys->CreateForwardEx(NULL, ET_Hook, 1, NULL, Param_String);
		pluginsys->AddPluginsListener(this);
	}
	virtual void OnSourceModShutdown()
	{
		pluginsys->RemovePluginsListener(this);
		forwardsys->ReleaseForward(g_pLogHook);
		handlesys->RemoveType(g_DirType, g_pCoreIdent);
		handlesys->RemoveType(g_FileType, g_pCoreIdent);
		handlesys->RemoveType(g_ValveFileType, g_pCoreIdent);
		handlesys->RemoveType(g_ValveDirType, g_pCoreIdent);
		g_DirType = 0;
		g_FileType = 0;
		g_ValveFileType = 0;
		g_ValveDirType = 0;
	}
	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == g_FileType)
		{
			FILE *fp = (FILE *)object;
			fclose(fp);
		}
		else if (type == g_DirType)
		{
			IDirectory *pDir = (IDirectory *)object;
			libsys->CloseDirectory(pDir);
		}
		else if (type == g_ValveFileType)
		{
			FileHandle_t fp = (FileHandle_t) object;
			smcore.filesystem->Close(fp);
		}
		else if (type == g_ValveDirType)
		{
			ValveDirectory *valveDir = (ValveDirectory *)object;
			smcore.filesystem->FindClose(valveDir->hndl);
			delete valveDir;
		}
	}
	virtual void AddLogHook(IPluginFunction *pFunc)
	{
		g_pLogHook->AddFunction(pFunc);
	}
	virtual void RemoveLogHook(IPluginFunction *pFunc)
	{
		g_pLogHook->RemoveFunction(pFunc);
	}
	virtual bool LogPrint(const char *msg)
	{
		cell_t result = 0;
		g_pLogHook->PushString(msg);
		g_pLogHook->Execute(&result);
		return result >= Pl_Handled;
	}
} s_FileNatives;

bool OnLogPrint(const char *msg)
{
	return s_FileNatives.LogPrint(msg);
}

static cell_t sm_OpenDirectory(IPluginContext *pContext, const cell_t *params)
{
	char *path;
	int err;
	if ((err=pContext->LocalToString(params[1], &path)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}
	
	Handle_t handle = 0;
	
	if (params[0] >= 2 && params[2])
	{
		char wildcardedPath[PLATFORM_MAX_PATH];
		snprintf(wildcardedPath, sizeof(wildcardedPath), "%s*", path);
		ValveDirectory *valveDir = new ValveDirectory;
		
		char *pathID;
		if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		const char *pFirst = smcore.filesystem->FindFirstEx(wildcardedPath, pathID, &valveDir->hndl);
		if (pFirst)
		{
			valveDir->bHandledFirstPath = false;
			strncpy(valveDir->szFirstPath, pFirst, sizeof(valveDir->szFirstPath));
		}
		else
		{
			valveDir->bHandledFirstPath = true;
		}
		
		handle = handlesys->CreateHandle(g_ValveDirType, valveDir, pContext->GetIdentity(), g_pCoreIdent, NULL);
	}
	else
	{
		char realpath[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);

		IDirectory *pDir = libsys->OpenDirectory(realpath);
		if (!pDir)
		{
			return 0;
		}

		handle = handlesys->CreateHandle(g_DirType, pDir, pContext->GetIdentity(), g_pCoreIdent, NULL);
	}
	
	return handle;
}

static cell_t sm_ReadDirEntry(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	void *pTempDir;
	HandleError herr;
	HandleSecurity sec;
	int err;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DirType, &sec, &pTempDir)) == HandleError_None)
	{
		IDirectory *pDir = (IDirectory *)pTempDir;
		if (!pDir->MoreFiles())
		{
			return 0;
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
			return pContext->ThrowNativeErrorEx(err, NULL);
		}

		pDir->NextEntry();
	}
	else if ((herr=handlesys->ReadHandle(hndl, g_ValveDirType, &sec, &pTempDir)) == HandleError_None)
	{
		ValveDirectory *valveDir = (ValveDirectory *)pTempDir;
		
		const char *pEntry = NULL;
		if (!valveDir->bHandledFirstPath)
		{
			if (valveDir->szFirstPath[0])
			{
				pEntry = valveDir->szFirstPath;
			}
		}
		else
		{
			pEntry = smcore.filesystem->FindNext(valveDir->hndl);
		}
		
		valveDir->bHandledFirstPath = true;
		
		// No more entries
		if (!pEntry)
		{
			return 0;
		}
		
		if ((err=pContext->StringToLocalUTF8(params[2], params[3], pEntry, NULL))
			!= SP_ERROR_NONE)
		{
			return pContext->ThrowNativeErrorEx(err, NULL);
		}

		cell_t *filetype;
		if ((err=pContext->LocalToPhysAddr(params[4], &filetype)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}

		if (smcore.filesystem->FindIsDirectory(valveDir->hndl))
		{
			*filetype = 1;
		} else {
			*filetype = 2;
		}		
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return 1;
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

	Handle_t handle = 0;
	HandleType_t handleType;
	FSHelper fshelper;
	const char *openpath;
	char *pathID;
	if (params[0] <= 2 || !params[3])
	{
		handleType = g_FileType;
		fshelper.SetFSType(FSType::STDIO);

		char realpath[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
		openpath = realpath;
	}
	else
	{
		if ((err=pContext->LocalToStringNULL(params[4], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		handleType = g_ValveFileType;
		fshelper.SetFSType(FSType::VALVE);
		openpath = name;
	}

	void *pFile = fshelper.Open(openpath, mode, pathID);
	if (pFile)
	{
		handle = handlesys->CreateHandle(handleType, pFile, pContext->GetIdentity(), g_pCoreIdent, NULL);
	}

	return handle;
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

	FSHelper fshelper;
	const char *filepath;
	char *pathID;
	if (params[0] < 2 || !params[2])
	{
		fshelper.SetFSType(FSType::STDIO);
		char realpath[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
		filepath = realpath;
	}
	else
	{
		if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		fshelper.SetFSType(FSType::VALVE);
		filepath = name;
	}

	return fshelper.Remove(filepath, pathID) ? 1 : 0;
}

static cell_t sm_ReadFileLine(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	int err;

	void *pFile;
	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	char *buf;
	if ((err=pContext->LocalToString(params[2], &buf)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	FSHelper fshelper;
	
	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return fshelper.ReadLine(buf, params[3], pFile) == NULL ? 0 : 1;
}

static cell_t sm_IsEndOfFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	void *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	FSHelper fshelper;
	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return fshelper.EndOfFile(pFile) ? 1 : 0;
}

static cell_t sm_FileSeek(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	void *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	FSHelper fshelper;
	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	fshelper.Seek(pFile, params[2], params[3]);
	
	return 1;
}

static cell_t sm_FilePosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	void *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	FSHelper fshelper;

	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return (cell_t)fshelper.Tell(pFile);
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

	if (params[0] >= 2 && params[2] == 1)
	{
		char *pathID = NULL;
		if (params[0] >= 3)
		{
			if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
			{
				pContext->ThrowNativeErrorEx(err, NULL);
				return 0;
			}
		}
		
		return smcore.filesystem->FileExists(name, pathID) ? 1 : 0;
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
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
#elif defined PLATFORM_POSIX
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
	
	if (params[0] >= 3 && params[3] == 1)
	{
		char *pathID;
		if ((err=pContext->LocalToStringNULL(params[4], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		smcore.filesystem->RenameFile(oldpath, newpath, pathID);
		return 1;
	}

	char new_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, new_realpath, sizeof(new_realpath), "%s", newpath);
	char old_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, old_realpath, sizeof(old_realpath), "%s", oldpath);

#ifdef PLATFORM_WINDOWS
	return (MoveFileA(old_realpath, new_realpath)) ? 1 : 0;
#elif defined PLATFORM_POSIX
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
	
	if (params[0] >= 2 && params[2] == 1)
	{
		char *pathID;
		if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		return smcore.filesystem->IsDirectory(name, pathID) ? 1 : 0;
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
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
#elif defined PLATFORM_POSIX
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

	if (params[0] >= 2 && params[2] == 1)
	{
		char *pathID = NULL;
		if (params[0] >= 3)
		{
			if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
			{
				pContext->ThrowNativeErrorEx(err, NULL);
				return -1;
			}
		}
		
		if (smcore.filesystem->FileExists(name, pathID))
		{
			return smcore.filesystem->Size(name, pathID);
		}
		else
		{
			return -1;
		}
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
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
#elif defined PLATFORM_POSIX
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

static cell_t sm_SetFilePermissions(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	char realpath[PLATFORM_MAX_PATH];

	pContext->LocalToString(params[1], &name);
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

#if defined PLATFORM_WINDOWS
	int mask = 0;
	if (params[2] & FPERM_U_WRITE || params[2] & FPERM_G_WRITE || params[2] & FPERM_O_WRITE)
	{
		mask |= _S_IWRITE;
	}
	if (params[2] & FPERM_U_READ || params[2] & FPERM_G_READ || params[2] & FPERM_O_READ ||
		params[2] & FPERM_U_EXEC || params[2] & FPERM_G_EXEC || params[2] & FPERM_O_EXEC)
	{
		mask |= _S_IREAD;
	}
	return _chmod(realpath, mask) == 0;
#else
	return chmod(realpath, params[2]) == 0;
#endif
}

static cell_t sm_CreateDirectory(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);
	
	if (params[0] >= 3 && params[3] == 1)
	{
		int err;
		char *pathID;
		if ((err=pContext->LocalToStringNULL(params[4], &pathID)) != SP_ERROR_NONE)
		{
			return pContext->ThrowNativeErrorEx(err, NULL);
		}
		
		if (smcore.filesystem->IsDirectory(name, pathID))
			return 0;
		
		smcore.filesystem->CreateDirHierarchy(name, pathID);
		
		if (smcore.filesystem->IsDirectory(name, pathID))
			return 1;
		
		return 0;
	}
	
	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

#if defined PLATFORM_WINDOWS
	return mkdir(realpath) == 0;
#else
	return mkdir(realpath, params[2]) == 0;
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

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

	return (rmdir(realpath)) ? 0 : 1;
}

static cell_t sm_WriteFileLine(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	void *pTempFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	char *fmt;
	int err;

	if ((err=pContext->LocalToString(params[2], &fmt)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	char buffer[2048];
	int arg = 3;
	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pTempFile)) == HandleError_None)
	{
		FILE *pFile = (FILE *) pTempFile;
		smcore.atcprintf(buffer, sizeof(buffer), fmt, pContext, params, &arg);
		fprintf(pFile, "%s\n", buffer);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pTempFile)) == HandleError_None)
	{
		FileHandle_t pFile = (FileHandle_t) pTempFile;
		smcore.atcprintf(buffer, sizeof(buffer), fmt, pContext, params, &arg);
		sprintf(buffer, "%s\n", buffer);
		smcore.filesystem->FPrint(pFile, buffer);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return 1;
}

static cell_t sm_FlushFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	void *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	FSHelper fshelper;
	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	return fshelper.Flush(pFile) ? 1 : 0;
}

static cell_t sm_BuildPath(IPluginContext *pContext, const cell_t *params)
{
	char path[PLATFORM_MAX_PATH], *fmt, *buffer;
	int arg = 5;
	pContext->LocalToString(params[2], &buffer);
	pContext->LocalToString(params[4], &fmt);

	smcore.atcprintf(path, sizeof(path), fmt, pContext, params, &arg);

	return g_pSM->BuildPath(Path_SM_Rel, buffer, params[3], "%s", path);
}

static cell_t sm_LogToGame(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len = g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	if (len >= sizeof(buffer)-2)
	{
		buffer[1022] = '\n';
		buffer[1023] = '\0';
	} else {
		buffer[len++] = '\n';
		buffer[len] = '\0';
	}

	smcore.LogToGame(buffer);

	return 1;
}

static cell_t sm_LogMessage(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	g_Logger.LogMessage("[%s] %s", pPlugin->GetFilename(), buffer);

	return 1;
}

static cell_t sm_LogError(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	g_Logger.LogError("[%s] %s", pPlugin->GetFilename(), buffer);

	return 1;
}

static cell_t sm_GetFileTime(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int err;
	if ((err=pContext->LocalToString(params[1], &name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	time_t time_val;
	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

	if (!libsys->FileTime(realpath, (FileTimeType)params[2], &time_val))
	{
		return -1;
	}

	return (cell_t)time_val;
}

static cell_t sm_LogToOpenFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	char buffer[2048];
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	g_Logger.LogToOpenFile(pFile, "[%s] %s", pPlugin->GetFilename(), buffer);

	return 1;
}

static cell_t sm_LogToOpenFileEx(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	FILE *pFile;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_FileType, &sec, (void **)&pFile))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	char buffer[2048];
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	g_Logger.LogToOpenFile(pFile, "%s", buffer);

	return 1;
}

static cell_t sm_ReadFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);
	size_t read = 0;

	void *pFile = NULL;
	FSHelper fshelper;

	if ((herr = handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr = handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	if (params[4] != 1 && params[4] != 2 && params[4] != 4)
	{
		return pContext->ThrowNativeError("Invalid size specifier (%d is not 1, 2, or 4)", params[4]);
	}

	cell_t *data;
	pContext->LocalToPhysAddr(params[2], &data);
		
	if (params[4] == 4)
	{
		read = fshelper.Read(data, sizeof(cell_t) * params[3], pFile);
	}
	else if (params[4] == 2)
	{
		uint16_t val;
		for (cell_t i = 0; i < params[3]; i++)
		{
			if (fshelper.Read(&val, sizeof(uint16_t), pFile) != 1)
			{
				break;
			}
			data[read += sizeof(uint16_t)] = val;
		}
	}
	else if (params[4] == 1)
	{
		uint8_t val;
		for (cell_t i = 0; i < params[3]; i++)
		{
			if (fshelper.Read(&val, sizeof(uint8_t), pFile) != 1)
			{
				break;
			}
			data[read += sizeof(uint8_t)] = val;
		}
	}

	if (read != ((size_t)params[3] * params[4]) && fshelper.HasError(pFile))
	{
		return -1;
	}

	return read / params[4];
}

static cell_t sm_ReadFileString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);
	void *pFile;
	cell_t num_read = 0;

	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	FSHelper fshelper;

	if ((herr=handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr=handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}
	
	if (params[4] != -1)
	{
		if (size_t(params[4]) > size_t(params[3]))
		{
			return pContext->ThrowNativeError("read_count (%u) is greater than buffer size (%u)",
				params[4],
				params[3]);
		}

		num_read = (cell_t)fshelper.Read(buffer, params[4], pFile);

		if (num_read != params[4] && fshelper.HasError(pFile))
		{
			return -1;
		}

		return num_read;
	}

	char val;
	while (1)
	{
		if (params[3] == 0 || num_read >= params[3] - 1)
		{
			break;
		}
		if (fshelper.Read(&val, sizeof(val), pFile) != 1)
		{
			if (fshelper.HasError(pFile))
			{
				return -1;
			}
			break;
		}
		if (val == '\0')
		{
			break;
		}
		if (params[3] > 0 && num_read < params[3] - 1)
		{
			buffer[num_read++] = val;
		}
	}

	if (params[3] > 0)
	{
		buffer[num_read] = '\0';
	}

	return num_read;
}

static cell_t sm_WriteFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);
	void *pFile;
	FSHelper fshelper;

	if ((herr=handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr=handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	cell_t *data;
	pContext->LocalToPhysAddr(params[2], &data);

	if (params[4] != 1 && params[4] != 2 && params[4] != 4)
	{
		return pContext->ThrowNativeError("Invalid size specifier (%d is not 1, 2, or 4)", params[4]);
	}

	/* :NOTE: This really isn't compatible with big endian but we will never have to worry about that. */

	if (params[4] == 4)
	{
		if (fshelper.Write(data, sizeof(cell_t) * params[3], pFile) != (sizeof(cell_t) * (size_t)params[3]))
		{
			return 0;
		}
	}
	else if (params[4] == 2)
	{
		for (cell_t i = 0; i < params[3]; i++)
		{
			if (fshelper.Write(&data[i], sizeof(int16_t), pFile) != sizeof(int16_t))
			{
				return 0;
			}
		}
	}
	else if (params[4] == 1)
	{
		for (cell_t i = 0; i < params[3]; i++)
		{
			if (fshelper.Write(&data[i], sizeof(int8_t), pFile) != sizeof(int8_t))
			{
				return 0;
			}
		}
	}

	return 1;
}

static cell_t sm_WriteFileString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);
	void *pFile;
	FSHelper fshelper;

	if ((herr=handlesys->ReadHandle(hndl, g_FileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::STDIO);
	}
	else if ((herr=handlesys->ReadHandle(hndl, g_ValveFileType, &sec, &pFile)) == HandleError_None)
	{
		fshelper.SetFSType(FSType::VALVE);
	}
	else
	{
		return pContext->ThrowNativeError("Invalid file handle %x (error %d)", hndl, herr);
	}

	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	size_t len = strlen(buffer);

	if (params[3])
	{
		len++;
	}

	return (fshelper.Write(buffer, len, pFile) == len) ? 1 : 0;
}

static cell_t sm_AddGameLogHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunction;

	if ((pFunction=pContext->GetFunctionById(params[1])) == NULL)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[1]);
	}

	s_FileNatives.AddLogHook(pFunction);
	
	return 1;
}

static cell_t sm_RemoveGameLogHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunction;

	if ((pFunction=pContext->GetFunctionById(params[1])) == NULL)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[1]);
	}

	s_FileNatives.RemoveLogHook(pFunction);

	return 1;
}

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
	{"BuildPath",				sm_BuildPath},
	{"LogToGame",				sm_LogToGame},
	{"LogMessage",				sm_LogMessage},
	{"LogError",				sm_LogError},
	{"FlushFile",				sm_FlushFile},
	{"GetFileTime",				sm_GetFileTime},
	{"LogToOpenFile",			sm_LogToOpenFile},
	{"LogToOpenFileEx",			sm_LogToOpenFileEx},
	{"ReadFile",				sm_ReadFile},
	{"ReadFileString",			sm_ReadFileString},
	{"WriteFile",				sm_WriteFile},
	{"WriteFileString",			sm_WriteFileString},
	{"AddGameLogHook",			sm_AddGameLogHook},
	{"RemoveGameLogHook",		sm_RemoveGameLogHook},
	{"CreateDirectory",			sm_CreateDirectory},
	{"SetFilePermissions",		sm_SetFilePermissions},
	{NULL,						NULL},
};
