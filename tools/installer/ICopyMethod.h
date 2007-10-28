#ifndef _INCLUDE_INSTALLER_COPY_METHOD_H_
#define _INCLUDE_INSTALLER_COPY_METHOD_H_

#include "platform_headers.h"

class ICopyProgress
{
public:
	virtual void UpdateProgress(float percent_complete) =0;
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
