#ifndef _INCLUDE_PERFORM_INSTALL_H_
#define _INCLUDE_PERFORM_INSTALL_H_

#include "InstallerMain.h"
#include "ICopyMethod.h"

void *DisplayPerformInstall(HWND hWnd);
void SetInstallMethod(ICopyMethod *pCopyMethod);

#endif //_INCLUDE_PERFORM_INSTALL_H_
