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
#include <atomic>
#include <condition_variable>
#include <deque>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <IHandleSys.h>
#include <ILibrarySys.h>
#include <IPluginSys.h>
#include <IForwardSys.h>
#include <ISourceMod.h>
#include <ITranslator.h>
#include "common_logic.h"
#include "Logger.h"
#include "sprintf.h"
#include <amtl/am-thread.h>
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
	virtual size_t Size() = 0;
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

	size_t Size() override {
		return (size_t)bridge->filesystem->Size(handle_);
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
#if defined(_WIN32)
		static thread_local bool invalid_fopen = false;
		static auto handler = [](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) { invalid_fopen = true; };
		auto old = _set_thread_local_invalid_parameter_handler(handler);
		invalid_fopen = false;
#endif
		FILE *fp = fopen(path, mode);
#if defined(_WIN32)
		_set_thread_local_invalid_parameter_handler(old);
		if (invalid_fopen)
			return NULL;
#endif

		if (!fp)
			return NULL;
		return new SystemFile(fp);
	}

	static bool Delete(const char *path) {
		return unlink(path) == 0;
	}

	size_t Size() override {
#ifdef PLATFORM_WINDOWS
		struct _stat s;
		int fd = _fileno(fp_);
		if (fd == -1)
			return -1;
		if (_fstat(fd, &s) != 0)
			return -1;
		if (s.st_mode & S_IFREG)
			return static_cast<size_t>(s.st_size);
		return -1;
#elif defined PLATFORM_POSIX
		struct stat s;
		int fd = fileno(fp_);
		if (fd == -1)
			return -1;
		if (fstat(fd, &s) != 0)
			return -1;
		if (S_ISREG(s.st_mode))
			return static_cast<size_t>(s.st_size);
		return -1;
#endif
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

enum FileAsyncResult
{
	FileAsync_Success = 0,
	FileAsync_NotFound,
	FileAsync_Error,
};

enum FileAsyncOperation
{
	FileAsync_Delete,
	FileAsync_Rename,
	FileAsync_Copy,
	FileAsync_FileExists,
	FileAsync_DirExists,
	FileAsync_FileSize,
	FileAsync_CreateDirectory,
	FileAsync_RemoveDirectory,
	FileAsync_Read,
	FileAsync_Write,
	FileAsync_OpenDirectory,
};

enum FileAsyncCallback
{
	FileAsync_ResultCallback,
	FileAsync_ExistsCallback,
	FileAsync_SizeCallback,
	FileAsync_ReadCallback,
	FileAsync_DirectoryCallback,
};

struct FileAsyncDirectoryEntry
{
	std::string name;
	cell_t type;
};

class SnapshotDirectory final : public IDirectory
{
public:
	explicit SnapshotDirectory(std::vector<FileAsyncDirectoryEntry> entries)
	 : entries_(std::move(entries)), index_(0)
	{
	}

	bool MoreFiles() override
	{
		return index_ < entries_.size();
	}

	void NextEntry() override
	{
		if (MoreFiles())
			index_++;
	}

	const char *GetEntryName() override
	{
		return MoreFiles() ? entries_[index_].name.c_str() : "";
	}

	bool IsEntryDirectory() override
	{
		return MoreFiles() && entries_[index_].type == 1;
	}

	bool IsEntryFile() override
	{
		return MoreFiles() && entries_[index_].type == 2;
	}

	bool IsEntryValid() override
	{
		return MoreFiles();
	}

private:
	std::vector<FileAsyncDirectoryEntry> entries_;
	size_t index_;
};

class FileAsyncTask
{
public:
	FileAsyncTask(FileAsyncOperation operation, FileAsyncCallback callback,
		IPluginFunction *function, IdentityToken_t *owner, const char *path,
		const char *realpath, cell_t data, const char *other_realpath = nullptr,
		cell_t mode = 0, std::vector<uint8_t> contents = {}, bool append = false,
		bool valve = false, const char *pathID = nullptr, const char *other_pathID = nullptr)
	 : operation_(operation), callback_(callback), function_(function), owner_(owner),
	   path_(path), realpath_(realpath), other_realpath_(other_realpath ? other_realpath : ""),
	   data_(data), mode_(mode), result_(FileAsync_Error), exists_(false), size_(-1),
	   contents_(std::move(contents)), append_(append), cancelled_(false), valve_(valve),
	   pathID_(pathID ? pathID : ""), other_pathID_(other_pathID ? other_pathID : "")
	{
	}

	void Run()
	{
		try
		{
			if (valve_)
			{
				RunValve();
				return;
			}

			switch (operation_)
			{
				case FileAsync_Delete:
					result_ = SystemFile::Delete(realpath_.c_str()) ? FileAsync_Success : ResultFromErrno();
					break;
				case FileAsync_Rename:
					result_ = Rename() ? FileAsync_Success : ResultFromErrno();
					break;
				case FileAsync_Copy:
					Copy();
					break;
				case FileAsync_FileExists:
					exists_ = IsRegularFile(realpath_.c_str());
					result_ = FileAsync_Success;
					break;
				case FileAsync_DirExists:
					exists_ = IsDirectory(realpath_.c_str());
					result_ = FileAsync_Success;
					break;
				case FileAsync_FileSize:
					FileSize();
					break;
				case FileAsync_CreateDirectory:
					result_ = CreateDirectory() ? FileAsync_Success : ResultFromErrno();
					break;
				case FileAsync_RemoveDirectory:
					result_ = rmdir(realpath_.c_str()) == 0 ? FileAsync_Success : ResultFromErrno();
					break;
				case FileAsync_Read:
					Read();
					break;
				case FileAsync_Write:
					Write();
					break;
				case FileAsync_OpenDirectory:
					OpenDirectory();
					break;
			}
		}
		catch (...)
		{
			contents_.clear();
			result_ = FileAsync_Error;
		}
	}

	void Deliver()
	{
		if (cancelled_ || !function_->IsRunnable())
			return;

		switch (callback_)
		{
			case FileAsync_ResultCallback:
				function_->PushCell(result_);
				function_->PushString(path_.c_str());
				function_->PushCell(data_);
				break;
			case FileAsync_ExistsCallback:
				function_->PushCell(exists_ ? 1 : 0);
				function_->PushString(path_.c_str());
				function_->PushCell(data_);
				break;
			case FileAsync_SizeCallback:
				function_->PushCell(result_);
				function_->PushString(path_.c_str());
				function_->PushCell(size_);
				function_->PushCell(data_);
				break;
			case FileAsync_ReadCallback:
				DeliverRead();
				break;
			case FileAsync_DirectoryCallback:
				DeliverDirectory();
				break;
		}
		function_->Execute(nullptr);
	}

	IdentityToken_t *owner() const
	{
		return owner_;
	}

	void Cancel()
	{
		cancelled_ = true;
	}

private:
	static FileAsyncResult ResultFromErrno()
	{
		return errno == ENOENT ? FileAsync_NotFound : FileAsync_Error;
	}

	static bool IsRegularFile(const char *path)
	{
#ifdef PLATFORM_WINDOWS
		struct _stat s;
		return _stat(path, &s) == 0 && (s.st_mode & S_IFREG);
#else
		struct stat s;
		return stat(path, &s) == 0 && S_ISREG(s.st_mode);
#endif
	}

	static bool IsDirectory(const char *path)
	{
#ifdef PLATFORM_WINDOWS
		struct _stat s;
		return _stat(path, &s) == 0 && (s.st_mode & S_IFDIR);
#else
		struct stat s;
		return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
#endif
	}

	bool Rename()
	{
#ifdef PLATFORM_WINDOWS
		return MoveFileExA(other_realpath_.c_str(), realpath_.c_str(),
			MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != 0;
#else
		return rename(other_realpath_.c_str(), realpath_.c_str()) == 0;
#endif
	}

	void Copy()
	{
		std::error_code ec;
		std::filesystem::copy_file(other_realpath_, realpath_,
			std::filesystem::copy_options::overwrite_existing, ec);
		if (!ec)
		{
			result_ = FileAsync_Success;
			return;
		}
		result_ = ec == std::errc::no_such_file_or_directory
			? FileAsync_NotFound
			: FileAsync_Error;
	}

	void FileSize()
	{
#ifdef PLATFORM_WINDOWS
		struct _stat s;
		if (_stat(realpath_.c_str(), &s) != 0)
#else
		struct stat s;
		if (stat(realpath_.c_str(), &s) != 0)
#endif
		{
			result_ = ResultFromErrno();
			return;
		}

#ifdef PLATFORM_WINDOWS
		if (s.st_mode & S_IFREG)
#else
		if (S_ISREG(s.st_mode))
#endif
		{
			size_ = static_cast<cell_t>(s.st_size);
			result_ = FileAsync_Success;
			return;
		}
		result_ = FileAsync_Error;
	}

	void RunValve()
	{
		switch (operation_)
		{
			case FileAsync_Delete:
				if (!bridge->filesystem->FileExists(realpath_.c_str(), pathID_.c_str()))
					result_ = FileAsync_NotFound;
				else
				{
					bridge->filesystem->RemoveFile(realpath_.c_str(), pathID_.c_str());
					result_ = bridge->filesystem->FileExists(realpath_.c_str(), pathID_.c_str())
						? FileAsync_Error : FileAsync_Success;
				}
				break;
			case FileAsync_Rename:
				if (!bridge->filesystem->FileExists(other_realpath_.c_str(), pathID_.c_str()))
				{
					result_ = FileAsync_NotFound;
					break;
				}
				bridge->filesystem->RenameFile(other_realpath_.c_str(), realpath_.c_str(), pathID_.c_str());
				result_ = !bridge->filesystem->FileExists(other_realpath_.c_str(), pathID_.c_str())
					&& bridge->filesystem->FileExists(realpath_.c_str(), pathID_.c_str())
					? FileAsync_Success : FileAsync_Error;
				break;
			case FileAsync_Copy:
				CopyValve();
				break;
			case FileAsync_FileExists:
				exists_ = bridge->filesystem->FileExists(realpath_.c_str(), pathID_.c_str());
				result_ = FileAsync_Success;
				break;
			case FileAsync_DirExists:
				exists_ = bridge->filesystem->IsDirectory(realpath_.c_str(), pathID_.c_str());
				result_ = FileAsync_Success;
				break;
			case FileAsync_FileSize:
				if (!bridge->filesystem->FileExists(realpath_.c_str(), pathID_.c_str()))
					result_ = FileAsync_NotFound;
				else
				{
					size_ = static_cast<cell_t>(bridge->filesystem->Size(realpath_.c_str(), pathID_.c_str()));
					result_ = FileAsync_Success;
				}
				break;
			case FileAsync_CreateDirectory:
				if (bridge->filesystem->IsDirectory(realpath_.c_str(), pathID_.c_str()))
					result_ = FileAsync_Error;
				else
				{
					bridge->filesystem->CreateDirHierarchy(realpath_.c_str(), pathID_.c_str());
					result_ = bridge->filesystem->IsDirectory(realpath_.c_str(), pathID_.c_str())
						? FileAsync_Success : FileAsync_Error;
				}
				break;
			default:
				result_ = FileAsync_Error;
				break;
		}
	}

	void CopyValve()
	{
		if (!bridge->filesystem->FileExists(other_realpath_.c_str(), other_pathID_.c_str()))
		{
			result_ = FileAsync_NotFound;
			return;
		}

		FileHandle_t source = bridge->filesystem->Open(other_realpath_.c_str(), "rb", other_pathID_.c_str());
		if (!source)
		{
			result_ = FileAsync_Error;
			return;
		}
		FileHandle_t destination = bridge->filesystem->Open(realpath_.c_str(), "wb", pathID_.c_str());
		if (!destination)
		{
			bridge->filesystem->Close(source);
			result_ = FileAsync_Error;
			return;
		}

		char buffer[8192];
		bool success = true;
		while (!bridge->filesystem->EndOfFile(source))
		{
			int read = bridge->filesystem->Read(buffer, sizeof(buffer), source);
			if (read < 0 || (read == 0 && !bridge->filesystem->EndOfFile(source)) ||
				(read && bridge->filesystem->Write(buffer, read, destination) != read))
			{
				success = false;
				break;
			}
		}
		success = success && bridge->filesystem->IsOk(source) && bridge->filesystem->IsOk(destination);
		bridge->filesystem->Close(destination);
		bridge->filesystem->Close(source);
		result_ = success ? FileAsync_Success : FileAsync_Error;
	}

	bool CreateDirectory()
	{
#ifdef PLATFORM_WINDOWS
		return mkdir(realpath_.c_str()) == 0;
#else
		return mkdir(realpath_.c_str(), mode_) == 0;
#endif
	}

	void Read()
	{
		std::error_code ec;
		auto size = std::filesystem::file_size(realpath_, ec);
		if (ec)
		{
			result_ = ec == std::errc::no_such_file_or_directory
				? FileAsync_NotFound
				: FileAsync_Error;
			return;
		}
		if (size > static_cast<uintmax_t>((std::numeric_limits<cell_t>::max)()))
		{
			result_ = FileAsync_Error;
			return;
		}

		contents_.resize(static_cast<size_t>(size));
		FILE *file = fopen(realpath_.c_str(), "rb");
		if (!file)
		{
			contents_.clear();
			result_ = ResultFromErrno();
			return;
		}

		size_t read = contents_.empty() ? 0 : fread(contents_.data(), 1, contents_.size(), file);
		bool success = read == contents_.size() && !ferror(file);
		if (fclose(file) != 0)
			success = false;
		if (!success)
		{
			contents_.clear();
			result_ = FileAsync_Error;
			return;
		}
		result_ = FileAsync_Success;
	}

	void Write()
	{
		FILE *file = fopen(realpath_.c_str(), append_ ? "ab" : "wb");
		if (!file)
		{
			result_ = ResultFromErrno();
			return;
		}

		size_t written = contents_.empty() ? 0 : fwrite(contents_.data(), 1, contents_.size(), file);
		bool success = written == contents_.size() && !ferror(file);
		if (fclose(file) != 0)
			success = false;
		result_ = success ? FileAsync_Success : FileAsync_Error;
	}

	void OpenDirectory()
	{
		IDirectory *directory = libsys->OpenDirectory(realpath_.c_str());
		if (!directory)
		{
			result_ = IsDirectory(realpath_.c_str()) ? FileAsync_Error : FileAsync_NotFound;
			return;
		}

		while (directory->MoreFiles())
		{
			FileAsyncDirectoryEntry entry;
			entry.name = directory->GetEntryName();
			entry.type = directory->IsEntryDirectory() ? 1 : (directory->IsEntryFile() ? 2 : 0);
			entries_.push_back(std::move(entry));
			directory->NextEntry();
		}
		libsys->CloseDirectory(directory);
		result_ = FileAsync_Success;
	}

	void DeliverRead()
	{
		char empty = '\0';
		size_t buffer_size = contents_.empty() ? 1 : contents_.size();
		function_->PushCell(result_);
		function_->PushString(path_.c_str());
		function_->PushStringEx(contents_.empty() ? &empty : reinterpret_cast<char *>(contents_.data()),
			buffer_size, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
		function_->PushCell(result_ == FileAsync_Success ? static_cast<cell_t>(contents_.size()) : 0);
		function_->PushCell(data_);
	}

	void DeliverDirectory()
	{
		Handle_t handle = 0;
		if (result_ == FileAsync_Success)
		{
			SnapshotDirectory *directory = new SnapshotDirectory(std::move(entries_));
			handle = handlesys->CreateHandle(g_DirType, directory, owner_, g_pCoreIdent, nullptr);
			if (!handle)
			{
				delete directory;
				result_ = FileAsync_Error;
			}
		}
		function_->PushCell(result_);
		function_->PushString(path_.c_str());
		function_->PushCell(handle);
		function_->PushCell(data_);
	}

private:
	FileAsyncOperation operation_;
	FileAsyncCallback callback_;
	IPluginFunction *function_;
	IdentityToken_t *owner_;
	std::string path_;
	std::string realpath_;
	std::string other_realpath_;
	cell_t data_;
	cell_t mode_;
	FileAsyncResult result_;
	bool exists_;
	cell_t size_;
	std::vector<uint8_t> contents_;
	std::vector<FileAsyncDirectoryEntry> entries_;
	bool append_;
	bool cancelled_;
	bool valve_;
	std::string pathID_;
	std::string other_pathID_;
};

class FileAsyncWorker
{
public:
	void Start()
	{
		thread_ = ke::NewThread("SM File Async", [this] { ThreadMain(); });
	}

	void Shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopping_ = true;
			for (FileAsyncTask *task : tasks_)
				task->Cancel();
		}
		condition_.notify_one();
		if (thread_)
		{
			thread_->join();
			thread_.reset();
		}

		std::lock_guard<std::mutex> lock(mutex_);
		for (FileAsyncTask *task : tasks_)
			delete task;
		tasks_.clear();
		pending_.clear();
		completed_.clear();
	}

	bool Enqueue(FileAsyncTask *task)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (stopping_)
			return false;
		tasks_.insert(task);
		pending_.push_back(task);
		condition_.notify_one();
		return true;
	}

	void ProcessCompleted()
	{
		std::deque<FileAsyncTask *> completed;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			completed.swap(completed_);
			for (FileAsyncTask *task : completed)
				tasks_.erase(task);
		}

		for (FileAsyncTask *task : completed)
		{
			task->Deliver();
			delete task;
		}
	}

	void CancelPlugin(IdentityToken_t *owner)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (FileAsyncTask *task : tasks_)
		{
			if (task->owner() == owner)
				task->Cancel();
		}
	}

private:
	void ThreadMain()
	{
		for (;;)
		{
			FileAsyncTask *task;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				condition_.wait(lock, [this] { return stopping_ || !pending_.empty(); });
				if (stopping_)
					return;
				task = pending_.front();
				pending_.pop_front();
			}

			task->Run();

			std::lock_guard<std::mutex> lock(mutex_);
			completed_.push_back(task);
		}
	}

private:
	std::mutex mutex_;
	std::condition_variable condition_;
	std::unique_ptr<std::thread> thread_;
	std::deque<FileAsyncTask *> pending_;
	std::deque<FileAsyncTask *> completed_;
	std::unordered_set<FileAsyncTask *> tasks_;
	bool stopping_ = false;
};

class ValveFileAsyncTask
{
public:
	ValveFileAsyncTask(FileAsyncOperation operation, IPluginFunction *function,
		IdentityToken_t *owner, const char *path, cell_t data, std::vector<uint8_t> contents = {})
	 : operation_(operation), function_(function), owner_(owner), path_(path), data_(data),
	   result_(FileAsync_Error), contents_(std::move(contents)), control_(nullptr)
	{
	}

	bool Start(const char *pathID, bool append = false)
	{
		int status;
		if (operation_ == FileAsync_Read)
			status = bridge->filesystem->AsyncRead(path_.c_str(), pathID, &control_);
		else
			status = bridge->filesystem->AsyncWrite(path_.c_str(), contents_.data(),
				static_cast<int>(contents_.size()), append, &control_);

		return status >= SourceMod::AsyncStatus_Ok && control_ != nullptr;
	}

	bool Process()
	{
		int status = bridge->filesystem->AsyncStatus(control_);
		if (status > SourceMod::AsyncStatus_Ok)
			return false;

		if (status == SourceMod::AsyncStatus_Ok && operation_ == FileAsync_Read)
		{
			void *data = nullptr;
			int size = 0;
			status = bridge->filesystem->AsyncGetResult(control_, &data, &size);
			if (status == SourceMod::AsyncStatus_Ok && size >= 0 && (size == 0 || data != nullptr))
			{
				contents_.resize(size);
				if (size)
					memcpy(contents_.data(), data, size);
			}
		}

		if (status == SourceMod::AsyncStatus_Ok)
			result_ = FileAsync_Success;
		else if (status == SourceMod::AsyncStatus_FileOpenError)
			result_ = FileAsync_NotFound;
		else
			result_ = FileAsync_Error;

		bridge->filesystem->AsyncRelease(control_);
		control_ = nullptr;
		Deliver();
		return true;
	}

	void Cancel()
	{
		if (control_)
		{
			bridge->filesystem->AsyncAbort(control_);
			bridge->filesystem->AsyncRelease(control_);
			control_ = nullptr;
		}
	}

	IdentityToken_t *owner() const
	{
		return owner_;
	}

private:
	void Deliver()
	{
		if (!function_->IsRunnable())
			return;

		if (operation_ == FileAsync_Read)
		{
			char empty = '\0';
			size_t buffer_size = contents_.empty() ? 1 : contents_.size();
			function_->PushCell(result_);
			function_->PushString(path_.c_str());
			function_->PushStringEx(contents_.empty() ? &empty : reinterpret_cast<char *>(contents_.data()),
				buffer_size, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
			function_->PushCell(result_ == FileAsync_Success ? static_cast<cell_t>(contents_.size()) : 0);
			function_->PushCell(data_);
		}
		else
		{
			function_->PushCell(result_);
			function_->PushString(path_.c_str());
			function_->PushCell(data_);
		}
		function_->Execute(nullptr);
	}

private:
	FileAsyncOperation operation_;
	IPluginFunction *function_;
	IdentityToken_t *owner_;
	std::string path_;
	cell_t data_;
	FileAsyncResult result_;
	std::vector<uint8_t> contents_;
	SourceMod::AsyncControl_t control_;
};

class ValveFileAsyncQueue
{
public:
	~ValveFileAsyncQueue()
	{
		Shutdown();
	}

	bool Enqueue(ValveFileAsyncTask *task, const char *pathID, bool append = false)
	{
		if (!bridge->filesystem->SupportsAsync() || !task->Start(pathID, append))
			return false;
		tasks_.push_back(task);
		return true;
	}

	void Process()
	{
		for (auto iter = tasks_.begin(); iter != tasks_.end();)
		{
			ValveFileAsyncTask *task = *iter;
			if (!task->Process())
			{
				++iter;
				continue;
			}
			delete task;
			iter = tasks_.erase(iter);
		}
	}

	void CancelPlugin(IdentityToken_t *owner)
	{
		for (auto iter = tasks_.begin(); iter != tasks_.end();)
		{
			ValveFileAsyncTask *task = *iter;
			if (task->owner() != owner)
			{
				++iter;
				continue;
			}
			task->Cancel();
			delete task;
			iter = tasks_.erase(iter);
		}
	}

	void Shutdown()
	{
		for (ValveFileAsyncTask *task : tasks_)
		{
			task->Cancel();
			delete task;
		}
		tasks_.clear();
	}

private:
	std::vector<ValveFileAsyncTask *> tasks_;
};

static FileAsyncWorker *g_FileAsyncWorker = nullptr;
static ValveFileAsyncQueue *g_ValveFileAsyncQueue = nullptr;

static void FileAsyncFrame(bool)
{
	g_FileAsyncWorker->ProcessCompleted();
	g_ValveFileAsyncQueue->Process();
}

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
		g_FileAsyncWorker = &m_AsyncWorker;
		g_ValveFileAsyncQueue = &m_ValveAsyncQueue;
		m_AsyncWorker.Start();
		g_pSM->AddGameFrameHook(&FileAsyncFrame);
	}
	virtual void OnSourceModShutdown()
	{
		g_pSM->RemoveGameFrameHook(&FileAsyncFrame);
		m_ValveAsyncQueue.Shutdown();
		m_AsyncWorker.Shutdown();
		g_FileAsyncWorker = nullptr;
		g_ValveFileAsyncQueue = nullptr;
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
		if (!g_pLogHook) {
			return false;
		}

		cell_t result = 0;
		g_pLogHook->PushString(msg);
		g_pLogHook->Execute(&result);
		return result >= Pl_Handled;
	}
	virtual void OnPluginWillUnload(IPlugin *plugin)
	{
		m_AsyncWorker.CancelPlugin(plugin->GetIdentity());
		m_ValveAsyncQueue.CancelPlugin(plugin->GetIdentity());
	}
	bool EnqueueAsyncTask(FileAsyncTask *task)
	{
		return m_AsyncWorker.Enqueue(task);
	}
	bool EnqueueValveAsyncTask(ValveFileAsyncTask *task, const char *pathID, bool append = false)
	{
		return m_ValveAsyncQueue.Enqueue(task, pathID, append);
	}

private:
	FileAsyncWorker m_AsyncWorker;
	ValveFileAsyncQueue m_ValveAsyncQueue;
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

	if (mode == nullptr || mode[0] == '\0') {
		return pContext->ThrowNativeError("File open mode cannot be empty!");
	}
	
	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') {
		return pContext->ThrowNativeError("File open mode is invalid \"%s\"!", mode);
	}

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

static cell_t QueueFileAsyncTask(IPluginContext *pContext, FileAsyncTask *task)
{
	if (s_FileNatives.EnqueueAsyncTask(task))
		return 0;

	delete task;
	return pContext->ThrowNativeError("Async file worker is shutting down");
}

static cell_t QueueValveWorkerFileAsyncTask(IPluginContext *pContext, FileAsyncTask *task)
{
	if (!bridge->filesystem->SupportsAsync())
	{
		delete task;
		return pContext->ThrowNativeError("Valve filesystem async operations are unavailable on this engine");
	}
	return QueueFileAsyncTask(pContext, task);
}

static cell_t QueueValveFileAsyncTask(IPluginContext *pContext, ValveFileAsyncTask *task,
	const char *pathID, bool append = false)
{
	if (s_FileNatives.EnqueueValveAsyncTask(task, pathID, append))
		return 0;

	delete task;
	return pContext->ThrowNativeError("Valve filesystem async operations are unavailable on this engine");
}

static IPluginFunction *GetFileAsyncCallback(IPluginContext *pContext, cell_t function)
{
	IPluginFunction *callback = pContext->GetFunctionById(function);
	if (!callback)
		pContext->ThrowNativeError("Invalid function id (%X)", function);
	return callback;
}

static cell_t sm_DeleteFileAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 4 && params[4])
	{
		char *pathID;
		pContext->LocalToString(params[5], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Delete,
			FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, path, params[3],
			nullptr, 0, {}, false, true, pathID));
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Delete,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_RenameFileAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[3]);
	if (!callback)
		return 0;

	char *newpath, *oldpath;
	pContext->LocalToString(params[1], &newpath);
	pContext->LocalToString(params[2], &oldpath);
	if (params[0] >= 5 && params[5])
	{
		char *pathID;
		pContext->LocalToString(params[6], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Rename,
			FileAsync_ResultCallback, callback, pContext->GetIdentity(), newpath, newpath, params[4],
			oldpath, 0, {}, false, true, pathID));
	}

	char new_realpath[PLATFORM_MAX_PATH], old_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, new_realpath, sizeof(new_realpath), "%s", newpath);
	g_pSM->BuildPath(Path_Game, old_realpath, sizeof(old_realpath), "%s", oldpath);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Rename,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), newpath, new_realpath,
		params[4], old_realpath));
}

static cell_t sm_CopyFileAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[3]);
	if (!callback)
		return 0;

	char *newpath, *oldpath;
	pContext->LocalToString(params[1], &newpath);
	pContext->LocalToString(params[2], &oldpath);
	if (params[0] >= 5 && params[5])
	{
		char *sourcePathID, *destinationPathID;
		pContext->LocalToString(params[6], &sourcePathID);
		pContext->LocalToString(params[7], &destinationPathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Copy,
			FileAsync_ResultCallback, callback, pContext->GetIdentity(), newpath, newpath, params[4],
			oldpath, 0, {}, false, true, destinationPathID, sourcePathID));
	}

	char new_realpath[PLATFORM_MAX_PATH], old_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, new_realpath, sizeof(new_realpath), "%s", newpath);
	g_pSM->BuildPath(Path_Game, old_realpath, sizeof(old_realpath), "%s", oldpath);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Copy,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), newpath, new_realpath,
		params[4], old_realpath));
}

static cell_t sm_FileExistsAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 4 && params[4])
	{
		char *pathID;
		pContext->LocalToString(params[5], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_FileExists,
			FileAsync_ExistsCallback, callback, pContext->GetIdentity(), path, path, params[3],
			nullptr, 0, {}, false, true, pathID));
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_FileExists,
		FileAsync_ExistsCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_DirExistsAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (!path[0])
		return pContext->ThrowNativeError("Invalid path. An empty path string is not valid, use \".\" to refer to the current working directory.");
	if (params[0] >= 4 && params[4])
	{
		char *pathID;
		pContext->LocalToString(params[5], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_DirExists,
			FileAsync_ExistsCallback, callback, pContext->GetIdentity(), path, path, params[3],
			nullptr, 0, {}, false, true, pathID));
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_DirExists,
		FileAsync_ExistsCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_FileSizeAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 4 && params[4])
	{
		char *pathID;
		pContext->LocalToString(params[5], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_FileSize,
			FileAsync_SizeCallback, callback, pContext->GetIdentity(), path, path, params[3],
			nullptr, 0, {}, false, true, pathID));
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_FileSize,
		FileAsync_SizeCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_ReadFileAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 4 && params[4])
	{
		char *pathID;
		pContext->LocalToString(params[5], &pathID);
		return QueueValveFileAsyncTask(pContext, new ValveFileAsyncTask(FileAsync_Read,
			callback, pContext->GetIdentity(), path, params[3]), pathID);
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Read,
		FileAsync_ReadCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_WriteFileAsync(IPluginContext *pContext, const cell_t *params)
{
	if (params[3] < 0)
		return pContext->ThrowNativeError("Invalid size %d", params[3]);

	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[4]);
	if (!callback)
		return 0;

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[2], &buffer);
	std::vector<uint8_t> contents(params[3]);
	if (!contents.empty())
		memcpy(contents.data(), buffer, contents.size());

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 7 && params[7])
	{
		char *pathID;
		pContext->LocalToString(params[8], &pathID);
		if (strcmp(pathID, "DEFAULT_WRITE_PATH") != 0)
			return pContext->ThrowNativeError("Valve async writes only support DEFAULT_WRITE_PATH");
		return QueueValveFileAsyncTask(pContext, new ValveFileAsyncTask(FileAsync_Write,
			callback, pContext->GetIdentity(), path, params[5], std::move(contents)), pathID,
			params[6] != 0);
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_Write,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, realpath,
		params[5], nullptr, 0, std::move(contents), params[6] != 0));
}

static cell_t sm_CreateDirectoryAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[3]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (params[0] >= 5 && params[5])
	{
		char *pathID;
		pContext->LocalToString(params[6], &pathID);
		return QueueValveWorkerFileAsyncTask(pContext, new FileAsyncTask(FileAsync_CreateDirectory,
			FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, path, params[4],
			nullptr, params[2], {}, false, true, pathID));
	}

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_CreateDirectory,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, realpath,
		params[4], nullptr, params[2]));
}

static cell_t sm_OpenDirectoryAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	if (!path[0])
		return pContext->ThrowNativeError("Invalid path. An empty path string is not valid, use \".\" to refer to the current working directory.");

	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_OpenDirectory,
		FileAsync_DirectoryCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
}

static cell_t sm_RemoveDirectoryAsync(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = GetFileAsyncCallback(pContext, params[2]);
	if (!callback)
		return 0;

	char *path;
	pContext->LocalToString(params[1], &path);
	char realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	return QueueFileAsyncTask(pContext, new FileAsyncTask(FileAsync_RemoveDirectory,
		FileAsync_ResultCallback, callback, pContext->GetIdentity(), path, realpath, params[3]));
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
	return (MoveFileExA(old_realpath, new_realpath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) ? 1 : 0;
#elif defined PLATFORM_POSIX
	return (rename(old_realpath, new_realpath)) ? 0 : 1;
#endif
}

// Streams the contents of one open file into another. Used for the Valve file
// system copy path, which has no native copy function.
static bool StreamCopyFile(FileObject *src, FileObject *dst)
{
	char buffer[8192];
	while (!src->EndOfFile())
	{
		size_t bytes = src->Read(buffer, sizeof(buffer));
		if (src->HasError())
			return false;
		if (bytes == 0)
			break;
		if (dst->Write(buffer, static_cast<int>(bytes)) != bytes)
			return false;
	}

	return !dst->HasError();
}

static cell_t sm_CopyFile(IPluginContext *pContext, const cell_t *params)
{
	char *newpath, *oldpath;
	pContext->LocalToString(params[1], &newpath);
	pContext->LocalToString(params[2], &oldpath);

	if (params[3] == 1)
	{
		char *sourcePathID;
		pContext->LocalToStringNULL(params[4], &sourcePathID);
		char *destPathID;
		pContext->LocalToStringNULL(params[5], &destPathID);

		ValveFile *src = ValveFile::Open(oldpath, "rb", sourcePathID);
		if (!src)
			return 0;

		ValveFile *dst = ValveFile::Open(newpath, "wb", destPathID);
		if (!dst)
		{
			delete src;
			return 0;
		}

		bool success = StreamCopyFile(src, dst);
		delete dst;
		delete src;
		return success ? 1 : 0;
	}

	char new_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, new_realpath, sizeof(new_realpath), "%s", newpath);
	char old_realpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, old_realpath, sizeof(old_realpath), "%s", oldpath);

	std::error_code ec;
	std::filesystem::copy_file(old_realpath, new_realpath,
		std::filesystem::copy_options::overwrite_existing, ec);
	return ec ? 0 : 1;
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

static cell_t sm_GetFilePermissions(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	char realpath[PLATFORM_MAX_PATH];

	pContext->LocalToString(params[1], &name);
	g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", name);

	cell_t *mask;
	pContext->LocalToPhysAddr(params[2], &mask);

#if defined PLATFORM_WINDOWS
	struct _stat buffer;
	cell_t valid = _stat(realpath, &buffer) == 0;

	if ((buffer.st_mode & _S_IREAD) != 0) {
		*mask |= (FPERM_U_READ|FPERM_G_READ|FPERM_O_READ)|(FPERM_U_EXEC|FPERM_G_EXEC|FPERM_O_EXEC);
	}

	if ((buffer.st_mode & _S_IWRITE) != 0) {
		*mask |= (FPERM_U_WRITE|FPERM_G_WRITE|FPERM_O_WRITE);
	}

	return valid;
#else
	struct stat buffer;
	cell_t valid = stat(realpath, &buffer) == 0;

	*mask = buffer.st_mode;
	return valid;
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

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext);
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

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext);
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

	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext);
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

static cell_t File_Size(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<FileObject> file(pContext, params[1], g_FileType);
	if (!file.Ok())
		return -1;

	return file->Size();
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
	{"OpenDirectoryAsync",		sm_OpenDirectoryAsync},
	{"ReadDirEntry",			sm_ReadDirEntry},
	{"OpenFile",				sm_OpenFile},
	{"DeleteFile",				sm_DeleteFile},
	{"DeleteFileAsync",			sm_DeleteFileAsync},
	{"ReadFileAsync",			sm_ReadFileAsync},
	{"WriteFileAsync",			sm_WriteFileAsync},
	{"ReadFileLine",			sm_ReadFileLine},
	{"IsEndOfFile",				sm_IsEndOfFile},
	{"FileSeek",				sm_FileSeek},
	{"FilePosition",			sm_FilePosition},
	{"FileExists",				sm_FileExists},
	{"RenameFile",				sm_RenameFile},
	{"RenameFileAsync",			sm_RenameFileAsync},
	{"CopyFile",				sm_CopyFile},
	{"CopyFileAsync",			sm_CopyFileAsync},
	{"DirExists",				sm_DirExists},
	{"DirExistsAsync",			sm_DirExistsAsync},
	{"FileSize",				sm_FileSize},
	{"FileSizeAsync",			sm_FileSizeAsync},
	{"RemoveDir",				sm_RemoveDir},
	{"RemoveDirectoryAsync",	sm_RemoveDirectoryAsync},
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
	{"CreateDirectoryAsync",		sm_CreateDirectoryAsync},
	{"FileExistsAsync",			sm_FileExistsAsync},
	{"SetFilePermissions",		sm_SetFilePermissions},
	{"GetFilePermissions",		sm_GetFilePermissions},

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
	{"File.Size",				File_Size},
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
