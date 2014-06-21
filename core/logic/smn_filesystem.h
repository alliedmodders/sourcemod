/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_FILESYSTEM_H_
#define _INCLUDE_SOURCEMOD_FILESYSTEM_H_

#define FPERM_U_READ		0x0100	/* User can read. */
#define FPERM_U_WRITE		0x0080	/* User can write. */
#define FPERM_U_EXEC		0x0040	/* User can exec. */
#define FPERM_G_READ		0x0020	/* Group can read. */
#define FPERM_G_WRITE		0x0010	/* Group can write. */
#define FPERM_G_EXEC		0x0008	/* Group can exec. */
#define FPERM_O_READ		0x0004	/* Anyone can read. */
#define FPERM_O_WRITE		0x0002	/* Anyone can write. */
#define FPERM_O_EXEC		0x0001	/* Anyone can exec. */

#endif //_INCLUDE_SOURCEMOD_FILESYSTEM_H_
