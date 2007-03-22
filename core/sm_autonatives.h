/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

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
