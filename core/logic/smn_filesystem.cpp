/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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

#include <assert.h>
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
#include "sprintf.h"
#include <am-utility.h>
#include "handle_helpers.h"
#include <bridge/include/IFileSystemBridge.h>
#include <bridge/include/CoreProvider.h>

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
HandleType_t g_DirType;
HandleType_t g_ValveDirType;
IChangeableForward *g_pLogHook = NULL;

class ValveFile;
class SystemFile;

class FileObject
{
public:
	virtual ~FileObject()
	{}
	virtual size_t Read(void *pOut, int size) = 0;
	virtual char *ReadLine(char *pOut, int size) = 0;
	virtual size_t Write(const void *pData, int size) = 0;
	virtual bool Seek(int pos, int seek_type) = 0;
	virtual int Tell() = 0;
	virtual bool Flush() = 0;
	virtual bool HasError() = 0;
	virtual bool EndOfFile() = 0;
	virtual void Close() = 0;
	virtual ValveFile *AsValveFile() {
		return NULL;
	}
	virtual SystemFile *AsSystemFile() {
		return NULL;
	}
};

class ValveFile : public FileObject
{
public:
	ValveFile(FileHandle_t handle)
	: handle_(handle)
	{}
	~ValveFile() {
		Close();
	}

	static ValveFile *Open(const char *filename, const char *mode, const char *pathID) {
		FileHandle_t handle = bridge->filesystem->Open(filename, mode, pathID);
		if (!handle)
			return NULL;
		return new ValveFile(handle);
	}

	static bool Delete(const char *filename, const char *pathID) {
		if (!bridge->filesystem->FileExists(filename, pathID))
			return false;

		bridge->filesystem->RemoveFile(filename, pathID);

		if (bridge->filesystem->FileExists(filename, pathID))
			return false;

		return true;
	}

	size_t Read(void *pOut, int size) override {
		return (size_t)bridge->filesystem->Read(pOut, size, handle_);
	}
	char *ReadLine(char *pOut, int size) override {
		return bridge->filesystem->ReadLine(pOut, size, handle_);
	}
	size_t Write(const void *pData, int size) override {
		return (size_t)bridge->filesystem->Write(pData, size, handle_);
	}
	bool Seek(int pos, int seek_type) override  {
		bridge->filesystem->Seek(handle_, pos, seek_type);
		return !HasError();
	}
	int Tell() override {
		return bridge->filesystem->Tell(handle_);
	}
	bool HasError() override {
		return !handle_ || !bridge->filesystem->IsOk(handle_);
	}
	bool Flush() override {
		bridge->filesystem->Flush(handle_);
		return true;
	}
	bool EndOfFile() override {
		return bridge->filesystem->EndOfFile(handle_);
	}
	void Close() override {
		if (!handle_)
			return;
		bridge->filesystem->Close(handle_);
		handle_ = NULL;
	}
	virtual ValveFile *AsValveFile() {
		return this;
	}
	FileHandle_t handle() const {
		return handle_;
	}

private:
	FileHandle_t handle_;
};

class SystemFile : public FileObject
{
public:
	SystemFile(FILE *fp)
	: fp_(fp)
	{}
	~SystemFile() {
		Close();
	}

	static SystemFile *Open(const char *path, const char *mode) {
		FILE *fp = fopen(path, mode);
		if (!fp)
			return NULL;
		return new SystemFile(fp);
	}

	static bool Delete(const char *path) {
		return unlink(path) == 0;
	}

	size_t Read(void *pOut, int size) override {
		return fread(pOut, 1, size, fp_);
	}
	char *ReadLine(char *pOut, int size) override {
		return fgets(pOut, size, fp_);
	}
	size_t Write(const void *pData, int size) override {
		return fwrite(pData, 1, size, fp_);
	}
	bool Seek(int pos, int seek_type) override  {
		return fseek(fp_, pos, seek_type) == 0;
	}
	int Tell() override {
		return ftell(fp_);
	}
	bool HasError() override {
		return ferror(fp_) != 0;
	}
	bool Flush() override {
		return fflush(fp_) == 0;
	}
	bool EndOfFile() override {
		return feof(fp_) != 0;
	}
	void Close() override {
		if (!fp_)
			return;
		fclose(fp_);
		fp_ = nullptr;
	}
	virtual SystemFile *AsSystemFile() {
		return this;
	}
	FILE *fp() const {
		return fp_;
	}

private:
	FILE *fp_;
};

struct ValveDirectory
{
	FileFindHandle_t hndl = -1;
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
		handlesys->RemoveType(g_ValveDirType, g_pCoreIdent);
		g_DirType = 0;
		g_FileType = 0;
		g_ValveDirType = 0;
	}
	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == g_FileType)
		{
			FileObject *file = (FileObject *)object;
			delete file;
		}
		else if (type == g_DirType)
		{
			IDirectory *pDir = (IDirectory *)object;
			libsys->CloseDirectory(pDir);
		}
		else if (type == g_ValveDirType)
		{
			ValveDirectory *valveDir = (ValveDirectory *)object;
			bridge->filesystem->FindClose(valveDir->hndl);
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
	
	if (!path[0])
	{
		return pContext->ThrowNativeError("Invalid path. An empty path string is not valid, use \".\" to refer to the current working directory.");
	}
	
	Handle_t handle = 0;
	
	if (params[0] >= 2 && params[2])
	{
		size_t len = strlen(path);
		char wildcardedPath[PLATFORM_MAX_PATH];
		ke::SafeSprintf(wildcardedPath, sizeof(wildcardedPath), "%s%s*", path, (path[len-1] != '/' && path[len-1] != '\\') ? "/" : "");
		
		char *pathID;
		if ((err=pContext->LocalToStringNULL(params[3], &pathID)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return 0;
		}
		
		ValveDirectory *valveDir = new ValveDirectory;
		
		const char *pFirst = bridge->filesystem->FindFirstEx(wildcardedPath, pathID, &valveDir->hndl);
		if (!pFirst)
		{
			delete valveDir;
			return 0;
		}
		else
		{
			valveDir->bHandledFirstPath = false;
			strncpy(valveDir->szFirstPath, pFirst, sizeof(valveDir->szFirstPath));
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
			pEntry = bridge->filesystem->FindNext(valveDir->hndl);
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

		if (bridge->filesystem->FindIsDirectory(valveDir->hndl))
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
	pContext->LocalToString(params[1], &name);
	pContext->LocalToString(params[2], &mode);

	FileObject *file = NULL;
	if (params[0] <= 2 || !params[3]) {
		char realpath[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
		file = SystemFile::Open(realpath, mode);
	} else {
		char *pathID;
		pContext->LocalToStringNULL(params[4], &pathID);
		file = ValveFile::Open(name, mode, pathID);
	}

	if (!file)
		return 0;

	Handle_t handle = handlesys->CreateHandle(g_FileType, file, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!handle) {
		delete file;
		return 0;
	}

	return handle;
}

static cell_t sm_DeleteFile(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	if (params[0] < 2 || !params[2])
	{
		char realpath[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
		return SystemFile::Delete(realpath);
	}
	else
	{
		char *pathID;
		pContext->LocalToStringNULL(params[3], &pathID);
		return ValveFile::Delete(name, pathID);
	}
}

static cell_t sm_ReadFileLine(IPluginContext *pContext, const cell_t *params)
{
	char *buf;
	pContext->LocalToString(params[2], &buf);

	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	return file->ReadLine(buf, params[3]) == NULL ? 0 : 1;
}

static cell_t sm_IsEndOfFile(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	return file->EndOfFile() ? 1 : 0;
}

static cell_t sm_FileSeek(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	return file->Seek(params[2], params[3]);
}

static cell_t sm_FilePosition(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	return file->Tell();
}

static cell_t sm_FileExists(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	if (params[0] >= 2 && params[2] == 1)
	{
		static char szDefaultPath[] = "GAME";
		char *pathID = szDefaultPath;
		if (params[0] >= 3)
			pContext->LocalToStringNULL(params[3], &pathID);
		
		return bridge->filesystem->FileExists(name, pathID) ? 1 : 0;
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
	pContext->LocalToString(params[1], &newpath);
	pContext->LocalToString(params[2], &oldpath);
	
	if (params[0] >= 3 && params[3] == 1)
	{
		char *pathID;
		pContext->LocalToStringNULL(params[4], &pathID);
		
		bridge->filesystem->RenameFile(oldpath, newpath, pathID);
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
	pContext->LocalToString(params[1], &name);

	if (!name[0])
	{
		return pContext->ThrowNativeError("Invalid path. An empty path string is not valid, use \".\" to refer to the current working directory.");
	}

	if (params[0] >= 2 && params[2] == 1)
	{
		char *pathID;
		pContext->LocalToStringNULL(params[3], &pathID);
		
		return bridge->filesystem->IsDirectory(name, pathID) ? 1 : 0;
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
	pContext->LocalToString(params[1], &name);

	if (params[0] >= 2 && params[2] == 1)
	{
		static char szDefaultPath[] = "GAME";
		char *pathID = szDefaultPath;
		if (params[0] >= 3)
			pContext->LocalToStringNULL(params[3], &pathID);
		
		if (!bridge->filesystem->FileExists(name, pathID))
			return -1;
		return bridge->filesystem->Size(name, pathID);
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(realpath, &s) != 0)
		return -1;
	if (s.st_mode & S_IFREG)
		return static_cast<cell_t>(s.st_size);
	return -1;
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(realpath, &s) != 0)
		return -1;
	if (S_ISREG(s.st_mode))
		return static_cast<cell_t>(s.st_size);
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
		char *pathID;
		pContext->LocalToStringNULL(params[4], &pathID);
		
		if (bridge->filesystem->IsDirectory(name, pathID))
			return 0;
		
		bridge->filesystem->CreateDirHierarchy(name, pathID);
		
		if (bridge->filesystem->IsDirectory(name, pathID))
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
	pContext->LocalToString(params[1], &name);

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

	return (rmdir(realpath)) ? 0 : 1;
}

static cell_t sm_WriteFileLine(IPluginContext *pContext, const cell_t *params)
{
	char *fmt;
	pContext->LocalToString(params[2], &fmt);

	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	int arg = 3;
	char buffer[2048];
	{
		DetectExceptions eh(pContext);
		atcprintf(buffer, sizeof(buffer), fmt, pContext, params, &arg);
		if (eh.HasException())
			return 0;
	}

	if (SystemFile *sysfile = file->AsSystemFile()) {
		fprintf(sysfile->fp(), "%s\n", buffer);
	} else if (ValveFile *vfile = file->AsValveFile()) {
		bridge->filesystem->FPrint(vfile->handle(), buffer);
		bridge->filesystem->FPrint(vfile->handle(), "\n");
	} else {
		assert(false);
	}

	return 1;
}

static cell_t sm_FlushFile(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	return file->Flush() ? 1 : 0;
}

static cell_t sm_BuildPath(IPluginContext *pContext, const cell_t *params)
{
	char path[PLATFORM_MAX_PATH], *fmt, *buffer;
	int arg = 5;
	pContext->LocalToString(params[2], &buffer);
	pContext->LocalToString(params[4], &fmt);

	{
		DetectExceptions eh(pContext);
		atcprintf(path, sizeof(path), fmt, pContext, params, &arg);
		if (eh.HasException())
			return 0;
	}

	return g_pSM->BuildPath(Path_SM_Rel, buffer, params[3], "%s", path);
}

static cell_t sm_LogToGame(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	size_t len;
	char buffer[1024];
	{
		DetectExceptions eh(pContext);
		len = g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);
		if (eh.HasException())
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

	bridge->LogToGame(buffer);

	return 1;
}

static cell_t sm_LogMessage(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);
		if (eh.HasException())
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
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);
		if (eh.HasException())
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
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	SystemFile *sysfile = file->AsSystemFile();
	if (!sysfile)
		return pContext->ThrowNativeError("Cannot log to files in the Valve file system");

	char buffer[2048];
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	g_Logger.LogToOpenFile(sysfile->fp(), "[%s] %s", pPlugin->GetFilename(), buffer);

	return 1;
}

static cell_t sm_LogToOpenFileEx(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	SystemFile *sysfile = file->AsSystemFile();
	if (!sysfile)
		return pContext->ThrowNativeError("Cannot log to files in the Valve file system");

	char buffer[2048];
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	g_Logger.LogToOpenFile(sysfile->fp(), "%s", buffer);
	return 1;
}

static cell_t sm_ReadFile(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	cell_t *data;
	pContext->LocalToPhysAddr(params[2], &data);
		
	size_t read = 0;
	switch (params[4]) {
		case 4:
			read = file->Read(data, sizeof(cell_t) * params[3]);
			break;

		case 2:
			for (cell_t i = 0; i < params[3]; i++) {
				uint16_t val;
				if (file->Read(&val, sizeof(val)) != sizeof(val))
					break;
				read += sizeof(val);
				*data++ = val;
			}
			break;

		case 1:
			for (cell_t i = 0; i < params[3]; i++) {
				uint8_t val;
				if (file->Read(&val, sizeof(val)) != sizeof(val))
					break;
				read += sizeof(val);
				*data++ = val;
			}
			break;

		default:
			return pContext->ThrowNativeError("Invalid size specifier (%d is not 1, 2, or 4)", params[4]);
	}

	if ((read != size_t(params[3] * params[4])) && file->HasError())
		return -1;

	return read / params[4];
}

static cell_t sm_ReadFileString(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	char *buffer;
	pContext->LocalToString(params[2], &buffer);
	
	cell_t num_read = 0;
	if (params[4] != -1) {
		if (size_t(params[4]) > size_t(params[3])) {
			return pContext->ThrowNativeError("read_count (%u) is greater than buffer size (%u)",
				params[4],
				params[3]);
		}

		num_read = (cell_t)file->Read(buffer, params[4]);
		if (num_read != params[4] && file->HasError())
			return -1;
		return num_read;
	}

	char val;
	while (1)
	{
		if (params[3] == 0 || num_read >= params[3] - 1)
			break;
		if (file->Read(&val, sizeof(val)) != 1) {
			if (file->HasError())
				return -1;
			break;
		}
		if (val == '\0')
			break;
		if (params[3] > 0 && num_read < params[3] - 1)
			buffer[num_read++] = val;
	}

	if (params[3] > 0)
		buffer[num_read] = '\0';

	return num_read;
}

static cell_t sm_WriteFile(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	cell_t *data;
	pContext->LocalToPhysAddr(params[2], &data);

	switch (params[4]) {
		case 4:
			if (file->Write(data, sizeof(cell_t) * params[3]) != sizeof(cell_t) * size_t(params[3]))
				return 0;
			break;

		case 2:
			for (cell_t i = 0; i < params[3]; i++) {
				int16_t v = data[i];
				if (file->Write(&v, sizeof(v)) != sizeof(v))
					return 0;
			}
			break;

		case 1:
			for (cell_t i = 0; i < params[3]; i++) {
				int8_t v = data[i];
				if (file->Write(&v, sizeof(v)) != sizeof(v))
					return 0;
			}
			break;

		default:
			return pContext->ThrowNativeError("Invalid size specifier (%d is not 1, 2, or 4)", params[4]);
	}

	return 1;
}

static cell_t sm_WriteFileString(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	size_t len = strlen(buffer);
	if (params[3])
		len++;

	return file->Write(buffer, len) >= len ? 1 : 0;
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

template <typename T>
static cell_t File_ReadTyped(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	cell_t *data;
	pContext->LocalToPhysAddr(params[2], &data);

	T value;
	if (file->Read(&value, sizeof(value)) != sizeof(value))
		return 0;

	*data = value;
	return 1;
}

template <typename T>
static cell_t File_WriteTyped(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return 0;

	T value = (T)params[2];
	return !!(file->Write(&value, sizeof(value)) == sizeof(value));
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

	{"File.ReadLine",			sm_ReadFileLine},
	{"File.Read",				sm_ReadFile},
	{"File.ReadString",			sm_ReadFileString},
	{"File.Write",				sm_WriteFile},
	{"File.WriteString",		sm_WriteFileString},
	{"File.WriteLine",			sm_WriteFileLine},
	{"File.EndOfFile",			sm_IsEndOfFile},
	{"File.Seek",				sm_FileSeek},
	{"File.Flush",				sm_FlushFile},
	{"File.Position.get",		sm_FilePosition},
	{"File.ReadInt8",			File_ReadTyped<int8_t>},
	{"File.ReadUint8",			File_ReadTyped<uint8_t>},
	{"File.ReadInt16",			File_ReadTyped<int16_t>},
	{"File.ReadUint16",			File_ReadTyped<uint16_t>},
	{"File.ReadInt32",			File_ReadTyped<int32_t>},
	{"File.WriteInt8",			File_WriteTyped<int8_t>},
	{"File.WriteInt16",			File_WriteTyped<int16_t>},
	{"File.WriteInt32",			File_WriteTyped<int32_t>},

	// Transitional syntax support.
	{"DirectoryListing.GetNext",			sm_ReadDirEntry},

	{NULL,						NULL},
};
