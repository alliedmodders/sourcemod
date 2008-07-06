#ifndef _INCLUDE_INSTALLER_COPY_METHOD_H_
#define _INCLUDE_INSTALLER_COPY_METHOD_H_

#include "platform_headers.h"

class ICopyProgress
{
public:
	virtual void StartingNewFile(const TCHAR *filename) =0;
	virtual void UpdateProgress(size_t bytes, size_t total_bytes) =0;
	virtual void FileDone(size_t file_size) =0;
};

class ICopyMethod
{
public:
	virtual bool CheckForExistingInstall() =0;
	virtual void TrackProgress(ICopyProgress *pProgress) =0;
	virtual bool SetCurrentFolder(const TCHAR *path, TCHAR *buffer, size_t maxchars) =0;
	virtual bool SendFile(const TCHAR *path, TCHAR *buffer, size_t maxchars) =0;
	virtual bool CreateFolder(const TCHAR *name, TCHAR *buffer, size_t maxchars) =0;
	virtual void CancelCurrentCopy() =0;
};

#endif //_INCLUDE_INSTALLER_COPY_METHOD_H_
