/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_VERSION_H_
#define _INCLUDE_SOURCEMOD_VERSION_H_

/**
 * @file Contains SourceMod version information.
 */

#define SM_BUILD_STRING		"$BUILD_STRING$"
#define SM_BUILD_UNIQUEID	"$BUILD_ID$" SM_BUILD_STRING
#define SVN_FULL_VERSION	"$PMAJOR$.$PMINOR$.$PREVISION$" SM_BUILD_STRING
#define SVN_FILE_VERSION	$PMAJOR$,$PMINOR$,$PREVISION$,0

#endif //_INCLUDE_SOURCEMOD_VERSION_H_
