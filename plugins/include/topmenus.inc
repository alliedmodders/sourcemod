/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 
#if defined _topmenus_included
 #endinput
#endif
#define _topmenus_included

#include <menus>

/**
 * Actions a top menu will take on an object.
 */
enum TopMenuAction
{
	/**
	 * An option is being drawn for a menu (or for sorting purposes).
	 *
	 * INPUT : TopMenu Handle, object ID, client index in extra parameter.
	 * OUTPUT: Buffer for rendering, maxlength of buffer.
	 * OUTPUT: Must return an ITEMDRAW constant.  The return value may be ignored 
	 *		   if the item is simply being sorted into the menu.
	 */
	TopMenuAction_DrawOption = 0,
	
	/**
	 * The title of a menu is being drawn for a given object.
	 *
	 * Note: The Object ID will be INVALID_TOPMENUOBJECT if drawing the 
	 * root title.  Otherwise, the Object ID is a category.
	 *
	 * INPUT : TopMenu Handle, object ID, client index in extra parameter.
	 * OUTPUT: Buffer for rendering, maxlength of buffer.
	 */
	TopMenuAction_DrawTitle = 1,
	
	/**
	 * A menu option has been selected.
	 *
	 * The Object ID will always be an item (not a category).
	 *
	 * INPUT : TopMenu Handle, object ID, client index in extra parameter.
	 */
	TopMenuAction_SelectOption = 2,
};

/**
 * Top menu object types.
 */
enum TopMenuObjectType
{
	TopMenuObject_Category = 0,			/**< Category (sub-menu branching from root) */
	TopMenuObject_Item = 1				/**< Item on a sub-menu */
};

/**
 * Top menu starting positions for display.
 */
enum TopMenuPosition
{
	TopMenuPosition_Start = 0,			/**< Start/root of the menu */
	TopMenuPosition_LastRoot = 1,		/**< Last position in the root menu */
	TopMenuPosition_LastCategory = 3,	/**< Last position in their last category */
};

/**
 * Top menu object tag for type checking.
 */
enum TopMenuObject
{
	INVALID_TOPMENUOBJECT = 0,
}

/**
 * TopMenu callback prototype.
 *
 * @param topmenu			Handle to the TopMenu.
 * @param action			TopMenuAction being performed.
 * @param object_id			The object ID (if used).
 * @param param				Extra parameter (if used).
 * @param buffer			Output buffer (if used).
 * @param maxlength			Output buffer (if used).
 * @noreturn
 */
functag TopMenuHandler public(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength);

/**
 * Creates a TopMenu.
 *
 * @param handler			Handler to use for drawing the root title.
 * @return					A new TopMenu Handle, or INVALID_HANDLE on failure.
 */						  
native Handle:CreateTopMenu(TopMenuHandler:handler);

/**
 * Re-sorts the items in a TopMenu via a configuration file.
 *
 * The format of the configuration file should be a Valve Key-Values 
 * formatted file that SourceMod can parse.  There should be one root 
 * section, and one sub-section for each category.  Each sub-section's 
 * name should match the category name.
 *
 * Each sub-section may only contain key/value pairs in the form of:
 * key: "item"
 * value: Name of the item as passed to AddToTopMenu().
 *
 * The TopMenu will draw items in the order declared in the configuration 
 * file.  If items do not appear in the configuration file, they are sorted 
 * per-player based on how the handler function renders for that player.  
 * These items appear after the configuration sorted items.
 *
 * @param topmenu			TopMenu Handle.
 * @param file				File path.
 * @param error				Error buffer.
 * @param maxlength			Maximum size of the error buffer.
 *							Error buffer will be filled with a 
 *							zero-terminated string if false is 
 *							returned.
 * @return					True on success, false on failure.
 * @error					Invalid TopMenu Handle.
 */
native bool:LoadTopMenuConfig(Handle:topmenu, const String:file[], String:error[], maxlength);

/**
 * Adds an object to a TopMenu.
 *
 * @param topmenu			TopMenu Handle.
 * @param name				Object name (MUST be unique).
 * @param type				Object type.
 * @param handler			Handler for object.
 * @param cmdname			Command name (for access overrides).
 * @param flags				Default access flags.
 * @param parent			Parent object ID, or INVALID_TOPMENUOBJECT for none.
 *							Items must have a category parent.
 *							Categories must not have a parent.
 * @return					A new TopMenuObject ID, or INVALID_TOPMENUOBJECT on
 *							failure.
 * @error					Invalid TopMenu Handle.
 */
native TopMenuObject:AddToTopMenu(Handle:topmenu,
								  const String:name[],
								  TopMenuObjectType:type,
								  TopMenuHandler:handler,
								  TopMenuObject:parent,
								  const String:cmdname[]="",
								  flags=0);

/**
 * Removes an object from a TopMenu.
 *
 * Plugins' objects are automatically removed all TopMenus when the given 
 * plugin unloads or pauses.  In the case of unpausing, all items are restored.
 *
 * @param topmenu			TopMenu Handle.
 * @param object			TopMenuObject ID.
 * @noreturn
 * @error					Invalid TopMenu Handle.
 */
native RemoveFromTopMenu(Handle:topmenu, TopMenuObject:object);

/**
 * Displays a TopMenu to a client.
 *
 * @param topmenu			TopMenu Handle.
 * @param client			Client index.
 * @param position			Position to display from.
 * @return					True on success, false on failure.
 * @error					Invalid TopMenu Handle or client not in game.
 */
native bool:DisplayTopMenu(Handle:topmenu, client, TopMenuPosition:position);

/**
 * Finds a category's object ID in a TopMenu.
 *
 * @param topmenu			TopMenu Handle.
 * @param name				Object's unique name.
 * @return					TopMenuObject ID on success, or 
 *							INVALID_TOPMENUOBJECT on failure.
 * @error					Invalid TopMenu Handle.
 */
native TopMenuObject:FindTopMenuCategory(Handle:topmenu, const String:name[]);

/**
 * Do not edit below this line!
 */
public Extension:__ext_cstrike = 
{
	name = "TopMenus",
	file = "topmenus.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

