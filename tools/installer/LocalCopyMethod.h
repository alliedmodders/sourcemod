#ifndef _INCLUDE_INSTALL_LOCAL_COPY_METHOD_H_
#define _INCLUDE_INSTALL_LOCAL_COPY_METHOD_H_

#include "platform_headers.h"
#include "ICopyMethod.h"

class LocalCopyMethod : public ICopyMethod
{
public:
	LocalCopyMethod();
public:
	virtual void TrackProgress(ICopyProgress *pProgress);
	virtual bool SetCurrentFolder(const TCHAR *path, TCHAR *buffer, size_t maxchars);
	virtual bool SendFile(const TCHAR *path, TCHAR *buffer, size_t maxchars);
	virtual bool CreateFolder(const TCHAR *name, TCHAR *buffer, size_t maxchars);
	virtual void CancelCurrentCopy();
	virtual bool CheckForExistingInstall();
public:
	void SetOutputPath(const TCHAR *path);
private:
	ICopyProgress *m_pProgress;
	TCHAR m_OutputPath[MAX_PATH];
	TCHAR m_CurrentPath[MAX_PATH];
	BOOL m_bCancelStatus;
};

extern LocalCopyMethod g_LocalCopier;

#endif //_INCLUDE_INSTALL_LOCAL_COPY_METHOD_H_
