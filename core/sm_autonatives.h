#ifndef _INCLUDE_SOURCEMOD_CORE_AUTONATIVES_H_
#define _INCLUDE_SOURCEMOD_CORE_AUTONATIVES_H_

#include "sm_globals.h"

#define REGISTER_NATIVES(globname) \
	extern sp_nativeinfo_t globNatives##globname[]; \
	CoreNativesToAdd globNativesCls##globname(globNatives##globname); \
	sp_nativeinfo_t globNatives##globname[] = 

class CoreNativesToAdd : public SMGlobalClass
{
public:
	CoreNativesToAdd(sp_nativeinfo_t *pList)
		: m_NativeList(pList)
	{
	}
	void OnSourceModAllInitialized();
	sp_nativeinfo_t *m_NativeList;
};

#endif //_INCLUDE_SOURCEMOD_CORE_AUTONATIVES_H_
