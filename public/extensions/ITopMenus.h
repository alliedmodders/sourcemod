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

#ifndef _INCLUDE_SOURCEMOD_MAIN_MENU_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MAIN_MENU_INTERFACE_H_

#include <IShareSys.h>
#include <ILibrarySys.h>
#include <IAdminSystem.h>
#include <IMenuManager.h>

/**
 * @file ITopMenus.h
 * @brief Interface header for creating and managing top-level menus.
 */

#define SMINTERFACE_TOPMENUS_NAME		"ITopMenus"
#define SMINTERFACE_TOPMENUS_VERSION	4

namespace SourceMod
{
	/**
	 * @brief Top menu object types.
	 */
	enum TopMenuObjectType
	{
		TopMenuObject_Category = 0,			/**< Category (sub-menu branching from root) */
		TopMenuObject_Item = 1				/**< Item on a sub-menu */
	};
	
	/**
	 * @brief Top menu starting positions for display.
	 */
	enum TopMenuPosition
	{
		TopMenuPosition_Start = 0,			/**< Start/root of the menu */
		TopMenuPosition_LastRoot = 1,		/**< Last position in the root menu */
		TopMenuPosition_LastCategory = 3,	/**< Last position in their last category */
	};

	class ITopMenu;

	/**
	 * @brief Top Menu callbacks for rendering/drawing.
	 */
	class ITopMenuObjectCallbacks
	{
	public:
		/**
		 * @brief Must return the topmenu API version.
		 *
		 * @return				Top Menu API version.
		 */
		virtual unsigned int GetTopMenuAPIVersion1()
		{
			return SMINTERFACE_TOPMENUS_VERSION;
		}

		/**
		 * @brief Requests how the given item should be drawn for a client.
		 *
		 * Unlike the other callbacks, this is only called in determining 
		 * whether to enable, disable, or ignore an item on a client's menu.
		 *
		 * @param menu			A pointer to the parent ITopMenu.
		 * @param client		Client index.
		 * @param object_id		Object ID returned from ITopMenu::AddToMenu().
		 * @return				ITEMDRAW flags to disable or not draw the 
		 *						option for this operation.
		 */
		virtual unsigned int OnTopMenuDrawOption(ITopMenu *menu,
			int client, 
			unsigned int object_id)
		{ 
			return ITEMDRAW_DEFAULT;
		}

		/**
		 * @brief Requests how the given item should be displayed for a client.
		 *
		 * This can be called either while drawing a menu or to decide how to 
		 * sort a menu for a player.
		 *
		 * @param menu			A pointer to the parent ITopMenu.
		 * @param client		Client index.
		 * @param object_id		Object ID returned from ITopMenu::AddToMenu().
		 * @param buffer		Buffer to store rendered text.
		 * @param maxlength		Maximum length of the rendering buffer.
		 */
		virtual void OnTopMenuDisplayOption(ITopMenu *menu,
			int client, 
			unsigned int object_id,
			char buffer[], 
			size_t maxlength) =0;

		/**
		 * @brief Requests how the given item's title should be displayed for
		 * a client.  This is called on any object_id that is a category.
		 *
		 * @param menu			A pointer to the parent ITopMenu.
		 * @param client		Client index.
		 * @param object_id		Object ID returned from ITopMenu::AddToMenu(), 
		 *						or 0 if the title is the root menu title.
		 * @param buffer		Buffer to store rendered text.
		 * @param maxlength		Maximum length of the rendering buffer.
		 */
		virtual void OnTopMenuDisplayTitle(ITopMenu *menu,
			int client,
			unsigned int object_id,
			char buffer[],
			size_t maxlength) =0;

		/**
		 * @brief Notifies the listener that the menu option has been selected.
		 *
		 * @param menu			A pointer to the parent ITopMenu.
		 * @param client		Client index.
		 * @param object_id		Object ID returned from ITopMenu::AddToMenu().
		 */
		virtual void OnTopMenuSelectOption(ITopMenu *menu, 
			int client, 
			unsigned int object_id) =0;

		/**
		 * @brief Notified when the given item is removed.
		 * 
		 * @param menu			A pointer to the parent ITopMenu.
		 * @param object_id		Object ID returned from ITopMenu::AddToMenu(), 
		 *						or 0 if the title callbacks are being removed.
		 */
		virtual void OnTopMenuObjectRemoved(ITopMenu *menu, unsigned int object_id) =0;
	};

	/**
	 * @brief "Top menu" interface, for managing top-level categorized menus.
	 */
	class ITopMenu
	{
	public:
		/**
		 * @brief Creates and adds an object type type to the top menu.
		 *
		 * @param name			Unique, string name to give the object.
		 * @param type			Object type.
		 * @param callbacks		ITopMenuObjectCallbacks pointer.
		 * @param owner			IdentityToken_t owner of the object.
		 * @param cmdname		Command name used for override access checks.
		 *						If NULL or empty, access will not be Checked.
		 * @param flags			Default flag(s) to use for access checks.
		 * @param parent		Parent object, or 0 if none.
		 *						Currently, categories cannot have a parent,
		 *						and items must have a category parent.
		 * @return				An object ID, or 0 on failure.
		 */
		virtual unsigned int AddToMenu(const char *name,
			TopMenuObjectType type,
			ITopMenuObjectCallbacks *callbacks,
			IdentityToken_t *owner,
			const char *cmdname,
			FlagBits flags,
			unsigned int parent) =0;

		/**
		 * @brief Removes an object from a menu.  If the object has any 
		 * children, those will be removed.
		 *
		 * @param object_id		Object ID returned from AddToMenu.
		 */
		virtual void RemoveFromMenu(unsigned int object_id) =0;

		/**
		 * @brief Sends the main menu to a given client.
		 *
		 * Once the menu is drawn to a client, the drawing order is cached. 
		 * If text on the menu is rendered differently for the client's next 
		 * viewing, the text will render properly, but its order will not
		 * change.  The menu is sorted by its configuration.  Remaining items 
		 * are sorted in alphabetical order using the initial display text.
		 *
		 * @param client		Client index.
		 * @param hold_time		Time to hold the menu on the screen for.
		 * @param position		TopMenuPosition enumeration value.
		 * @return				True on success, false if nothing displayed.
		 */
		virtual bool DisplayMenu(int client, unsigned int hold_time, TopMenuPosition position) =0;

		/**
		 * @brief Loads a configuration file for organizing the menu.  This 
		 * forces all known categories to be re-sorted.  
		 *
		 * Only one configuration can be active at a time.  Loading a new one 
		 * will cause the old sorting to disappear.
		 *
		 * @param file			File path.
		 * @param error			Error buffer.
		 * @param maxlength		Maximum length of the error buffer.
		 * @return				True on success, false on failure.
		 */
		virtual bool LoadConfiguration(const char *file, char *error, size_t maxlength) =0;

		/**
		 * @brief Finds a category's ID by name.
		 *
		 * @param name			Category's name.
		 * @return				Object ID of the category, or 0 if none.
		 */
		virtual unsigned int FindCategory(const char *name) =0;

		/**
		 * @brief Creates and adds an object type type to the top menu.
		 *
		 * @param name			Unique, string name to give the object.
		 * @param type			Object type.
		 * @param callbacks		ITopMenuObjectCallbacks pointer.
		 * @param owner			IdentityToken_t owner of the object.
		 * @param cmdname		Command name used for override access checks.
		 *						If NULL or empty, access will not be Checked.
		 * @param flags			Default flag(s) to use for access checks.
		 * @param parent		Parent object, or 0 if none.
		 *						Currently, categories cannot have a parent,
		 *						and items must have a category parent.
		 * @param info_string	Optional info string to attach to the object.
		 *						Only 255 bytes of the string (including null 
		 *						terminator) will be stored.
		 * @return				An object ID, or 0 on failure.
		 */
		virtual unsigned int AddToMenu2(const char *name,
			TopMenuObjectType type,
			ITopMenuObjectCallbacks *callbacks,
			IdentityToken_t *owner,
			const char *cmdname,
			FlagBits flags,
			unsigned int parent,
			const char *info_string) =0;

		/**
		 * @brief Returns an object's info string.
		 *
		 * @param object_id		Object ID.
		 * @return				Object's info string, or NULL if none.
		 */
		virtual const char *GetObjectInfoString(unsigned int object_id) =0;

		/**
		 * @brief Returns an object's name string.
		 *
		 * @param object_id		Object ID.
		 * @return				Object's name string, or NULL if none.
		 */
		virtual const char *GetObjectName(unsigned int object_id) =0;
	};

	/**
	 * @brief Top menu manager.
	 */
	class ITopMenuManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName() =0;
		virtual unsigned int GetInterfaceVersion() =0;
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version < 2)
			{
				return false;
			}
			return SMInterface::IsVersionCompatible(version);
		}
	public:
		/**
		 * @brief Creates a new top-level menu.
		 * 
		 * @param callbacks		Callbacks for the title text.
		 *						The object_id for the title will always be 0.
		 * @return				A new ITopMenu pointer.
		 */
		virtual ITopMenu *CreateTopMenu(ITopMenuObjectCallbacks *callbacks) =0;

		/**
		 * @brief Destroys a top-level menu.
		 *
		 * @param topmenu		Pointer to an ITopMenu.
		 */
		virtual void DestroyTopMenu(ITopMenu *topmenu) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_MAIN_MENU_INTERFACE_H_

