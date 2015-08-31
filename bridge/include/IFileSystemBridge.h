// vim: set ts=4 sw=4 tw=99 noet:
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_IFILESYSTEM_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_IFILESYSTEM_H_

typedef void * FileHandle_t;
typedef int FileFindHandle_t; 

namespace SourceMod {

class IFileSystemBridge
{
public:
	virtual const char *FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle) = 0;
	virtual const char *FindNext(FileFindHandle_t handle) = 0;
	virtual bool FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void FindClose(FileFindHandle_t handle) = 0;
	virtual FileHandle_t Open(const char *pFileName, const char *pOptions, const char *pathID = 0) = 0;
	virtual void Close(FileHandle_t file) = 0;
	virtual char *ReadLine(char *pOutput, int maxChars, FileHandle_t file) = 0;
	virtual bool EndOfFile(FileHandle_t file) = 0;
	virtual bool FileExists(const char *pFileName, const char *pPathID = 0) = 0;
	virtual unsigned int Size(const char *pFileName, const char *pPathID = 0) = 0;
	virtual int Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual void Seek(FileHandle_t file, int post, int seekType) = 0;
	virtual unsigned int Tell(FileHandle_t file) = 0;
	virtual int FPrint(FileHandle_t file, const char *pData) = 0;
	virtual void Flush(FileHandle_t file) = 0;
	virtual bool IsOk(FileHandle_t file) = 0;
	virtual void RemoveFile(const char *pRelativePath, const char *pathID = 0) = 0;
	virtual void RenameFile(char const *pOldPath, char const *pNewPath, const char *pathID = 0) = 0;
	virtual bool IsDirectory(const char *pFileName, const char *pathID = 0) = 0;
	virtual void CreateDirHierarchy(const char *path, const char *pathID = 0) = 0;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_BRIDGE_IFILESYSTEM_H_
