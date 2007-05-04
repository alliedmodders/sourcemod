/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "extension.h"
#include "CallMaker.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

BinTools g_BinTools;		/**< Global singleton for your extension's main interface */
CallMaker g_CallMaker;
ISourcePawnEngine *g_SPEngine;

SMEXT_LINK(&g_BinTools);

bool BinTools::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	g_SPEngine = g_pSM->GetScriptingEngine();
	g_pShareSys->AddInterface(myself, &g_CallMaker);

	return true;
}
