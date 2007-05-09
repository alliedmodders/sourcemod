/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "CallMaker.h"

/**
 * @file extension.cpp
 * @brief Implements BinTools extension code.
 */

BinTools g_BinTools;		/**< Global singleton for extension's main interface */
CallMaker g_CallMaker;
ISourcePawnEngine *g_SPEngine;

SMEXT_LINK(&g_BinTools);

bool BinTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	g_SPEngine = g_pSM->GetScriptingEngine();
	g_pShareSys->AddInterface(myself, &g_CallMaker);

	return true;
}
