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

#ifndef _INCLUDE_SOURCEMOD_MENU_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_MENU_SYSTEM_H_

#include <IShareSys.h>
#include <IHandleSys.h>

#define SMINTERFACE_MENUMANAGER_NAME		"IMenuManager"
#define SMINTERFACE_MENUMANAGER_VERSION		17

/**
 * @file IMenuManager.h
 * @brief Abstracts on-screen menus for clients.
 */

namespace SourceMod
{
	/**
	 * @brief Used to determine how an item selection is interpreted.
	 */
	enum ItemSelection
	{
		ItemSel_None,					/**< Invalid selection */
		ItemSel_Back,					/**< Go back one page (really "Previous") */
		ItemSel_Next,					/**< Go forward one page */
		ItemSel_Exit,					/**< Menu was exited */
		ItemSel_Item,					/**< Valid item selection */
		ItemSel_ExitBack,				/**< Sends MenuEnd_ExitBack */
	};

	/**
	 * @brief Used to determine which order to search for items in.
	 */
	enum ItemOrder
	{
		ItemOrder_Ascending,			/**< Items should be drawn ascendingly */
		ItemOrder_Descending,			/**< Items should be drawn descendingly */
	};

	/**
	 * @brief Pairs an item type with an item menu position.
	 */
	struct menu_slots_t
	{
		ItemSelection type;				/**< Item selection type */
		unsigned int item;				/**< Item position, if applicable */
	};

	class IBaseMenu;
	class IMenuPanel;
	class IMenuHandler;

	/**
	 * @brief Describes menu display information.
	 */
	struct menu_states_t
	{
		unsigned int apiVers;			/**< Must be filled with the API version */
		IBaseMenu *menu;				/**< Menu pointer, or NULL if there is only a display */
		IMenuHandler *mh;				/**< Menu callbacks handler */
		unsigned int firstItem;			/**< MENU ONLY: First item displayed on the last page */
		unsigned int lastItem;			/**< MENU ONLY: Last item displayed on the last page */
		unsigned int item_on_page;		/**< MENU ONLY: First item on page */
		menu_slots_t slots[11];			/**< MENU ONLY: Item selection table (first index is 1) */
	};

	#define ITEMDRAW_DEFAULT		(0)		/**< Item should be drawn normally */
	#define ITEMDRAW_DISABLED		(1<<0)	/**< Item is drawn but not selectable */
	#define ITEMDRAW_RAWLINE		(1<<1)	/**< Item should be a raw line, without a slot */
	#define ITEMDRAW_NOTEXT			(1<<2)	/**< No text should be drawn */
	#define ITEMDRAW_SPACER			(1<<3)	/**< Item should be drawn as a spacer, if possible */
	#define ITEMDRAW_IGNORE	((1<<1)|(1<<2))	/**< Item should be completely ignored (rawline + notext) */
	#define ITEMDRAW_CONTROL		(1<<4)	/**< Item is control text (back/next/exit) */

	/**
	 * @brief Information about item drawing.
	 */
	struct ItemDrawInfo
	{
		ItemDrawInfo(const char *DISPLAY=NULL, unsigned int STYLE=ITEMDRAW_DEFAULT, 
					 unsigned int FLAGS=0, const char *HELPTEXT=NULL) 
			: display(DISPLAY), style(STYLE)
		{
		}
		const char *display;			/**< Display text (NULL for none) */
		unsigned int style;				/**< ITEMDRAW style flags */
	};

	/**
	 * @brief Contains information about a vote result.
	 */
	struct menu_vote_result_t
	{
		unsigned int num_clients;		/**< Number of clients the menu was displayed to */
		unsigned int num_votes;			/**< Number of votes received */
		struct menu_client_vote_t
		{
			int client;					/**< Client index */
			int item;					/**< Item # (or -1 for none) */
		} *client_list;					/**< Array of size num_clients */
		unsigned int num_items;			/**< Number of items voted for */
		struct menu_item_vote_t
		{
			unsigned int item;			/**< Item index */
			unsigned int count;			/**< Number of votes */
		} *item_list;					/**< Array of size num_items, sorted by count, 
											 descending */
	};

	/**
	 * @brief Reasons for a menu dying.
	 */
	enum MenuCancelReason
	{
		MenuCancel_Disconnected = -1,	/**< Client dropped from the server */
		MenuCancel_Interrupted = -2,	/**< Client was interrupted with another menu */
		MenuCancel_Exit = -3,			/**< Client selected "exit" on a paginated menu */
		MenuCancel_NoDisplay = -4,		/**< Menu could not be displayed to the client */
		MenuCancel_Timeout = -5,		/**< Menu timed out */
		MenuCancel_ExitBack = -6,		/**< Client selected "exit back" on a paginated menu */
	};

	/**
	 * @brief Reasons a menu ended.
	 */
	enum MenuEndReason
	{
		MenuEnd_Selected = 0,				/**< Menu item was selected */
		MenuEnd_VotingDone = -1,			/**< Voting finished */
		MenuEnd_VotingCancelled = -2,		/**< Voting was cancelled */
		MenuEnd_Cancelled = -3,				/**< Menu was uncleanly cancelled */
		MenuEnd_Exit = -4,					/**< Menu was cleanly exited via "exit" */
		MenuEnd_ExitBack = -5,				/**< Menu was cleanly exited via "back" */
	};

	/**
	 * @brief Reasons a vote can be cancelled.
	 */
	enum VoteCancelReason
	{
		VoteCancel_Generic = -1,		/**< Vote was generically cancelled. */
		VoteCancel_NoVotes = -2,		/**< Vote did not receive any votes. */
	};

	#define MENU_NO_PAGINATION			0		/**< Menu should not be paginated (10 items max) */
	#define MENU_TIME_FOREVER			0		/**< Menu should be displayed as long as possible */

	#define MENUFLAG_BUTTON_EXIT		(1<<0)	/**< Menu has an "exit" button */
	#define MENUFLAG_BUTTON_EXITBACK	(1<<1)	/**< Menu has an "exit back" button */
	#define MENUFLAG_NO_SOUND			(1<<2)	/**< Menu will not have any select sounds */
	#define MENUFLAG_BUTTON_NOVOTE		(1<<3)	/**< Menu has a "No Vote" button at slot 1 */

	#define VOTEFLAG_NO_REVOTES			(1<<0)	/**< Players cannot change their votes */

	/**
	 * @brief Extended menu options.
	 */
	enum MenuOption
	{
		MenuOption_IntroMessage,				/**< CONST CHAR *: Valve menus only; defaults to:
													 "You have a menu, hit ESC"
													 */
		MenuOption_IntroColor,					/**< INT[4]: Valve menus only; specifies the intro message colour
													 using R,G,B,A (defaults to 255,0,0,255)
													 */
		MenuOption_Priority,					/**< INT *: Valve menus only; priority (less is higher) */
	};

	/**
	 * @brief Describes the menu a player is viewing.
	 */
	enum MenuSource
	{
		MenuSource_None = 0,					/**< No menu is being displayed */
		MenuSource_External = 1,				/**< External menu, no pointer */
		MenuSource_BaseMenu = 2,				/**< An IBaseMenu pointer. */
		MenuSource_Display = 3,					/**< IMenuPanel source, no pointer */
	};

	class IMenuStyle;

	/**
	 * @brief Sets how a raw menu should be drawn.
	 */
	class IMenuPanel
	{
	public:
		/**
		 * @brief Returns the parent IMenuStyle pointer.
		 *
		 * @return				IMenuStyle pointer which created
		 *						this object.
		 */
		virtual IMenuStyle *GetParentStyle() =0;

		/**
		 * @brief Resets/clears the cached display text.
		 */
		virtual void Reset() =0;

		/**
		 * @brief Sets how the title should be drawn.
		 *
		 * @param text			Text string to display for the title.
		 * @param onlyIfEmpty	Only sets the title if one does not already 
		 *						exist.
		 */
		virtual void DrawTitle(const char *text, bool onlyIfEmpty=false) =0;

		/**
		 * @brief Adds an item to the menu and returns the position (1-10).
		 *
		 * Note: Item will fail to draw if there are too many items,
		 * or the item is not drawable (for example, invisible).	
		 * 
		 * @return				Item draw position, or 0 on failure.
		 */
		virtual unsigned int DrawItem(const ItemDrawInfo &item) =0;

		/**
		 * @brief Draws a raw line of text, if supported.  The line does not 
		 * need to be newline terminated.
		 *
		 * @return				True on success, false if not supported.
		 */
		virtual bool DrawRawLine(const char *rawline) =0;

		/**
		 * @brief Sets an extended menu option.
		 *
		 * @param option		Option type.
		 * @param valuePtr		Pointer of the type expected by the option.
		 * @return				True on success, false if option or value is not supported.
		 */
		virtual bool SetExtOption(MenuOption option, const void *valuePtr) =0;

		/**
		 * @brief Returns whether the display is capable of rendering an item
		 * with the given flags.
		 *
		 * @param drawFlags		ITEMDRAW flags.
		 * @return				True if renderable, false otherwise.
		 */
		virtual bool CanDrawItem(unsigned int drawFlags) =0;

		/**
		 * @brief Sends the menu display to a client.
		 *
		 * @param client		Client index to display to.
		 * @param handler		Menu handler to use.
		 * @param time			Time to hold menu for.
		 * @return				True on success, false otherwise.
		 */
		virtual bool SendDisplay(int client, IMenuHandler *handler, unsigned int time) =0;

		/**
		 * @brief Destroys the display object.
		 */
		virtual void DeleteThis() =0;

		/**
		 * @brief Sets the selectable key map.  Returns false if the function 
		 * is not supported.
		 * 
		 * @param keymap		A bit string where each bit N-1 specifies 
		 *						that key N is selectable (key 0 is bit 9).  
		 *						If the selectable key map is 0, it will be
		 *						automatically set to allow 0.
		 * @return				True on success, false if not supported.
		 */
		virtual bool SetSelectableKeys(unsigned int keymap) =0;

		/**
		 * @brief Returns the current key position.
		 *
		 * @return				Current key position starting at 1.
		 */
		virtual unsigned int GetCurrentKey() =0;

		/**
		 * @brief Sets the next key position.  This cannot be used
		 * to traverse backwards.
		 *
		 * @param key			Key that is greater or equal to
		 *						GetCurrentKey().
		 * @return				True on success, false otherwise.
		 */
		virtual bool SetCurrentKey(unsigned int key) =0;

		/**
		 * @brief Returns the number of characters that can be added to the 
		 * menu.  The internal buffer is truncated if overflowed; this is for 
		 * manual truncation/wrapping purposes.
		 *
		 * @return				Number of bytes available.  If the result is 
		 *						-1, then the panel has no text limit.
		 */
		virtual int GetAmountRemaining() =0;

		/**
		 * @brief For the Handle system, returns approximate memory usage.
		 *
		 * @return				Approximate number of bytes being used.
		 */
		virtual unsigned int GetApproxMemUsage() =0;
		
		/**
		 * @brief Sets panel content directly
		 *
		 * @param str			New panel contents.
		 * @return				True if supported, otherwise false.
		 */
		virtual bool DirectSet(const char *str) =0;
	};

	/**
	 * @brief Describes a "MenuStyle" system which manages
	 * menu drawing and construction.
	 */
	class IMenuStyle
	{
	public:
		/**
		 * @brief Returns the style API version.
		 *
		 * @return				API version.
		 */
		virtual unsigned int GetStyleAPIVersion()
		{
			return SMINTERFACE_MENUMANAGER_VERSION;
		}

		/**
		 * @brief Returns the name of the menu style.
		 *
		 * @return				String containing the style name.
		 */
		virtual const char *GetStyleName() =0;

		/**
		 * @brief Creates an IMenuPanel object.
		 *
		 * Note: the object should be freed using ::DeleteThis.
		 *
		 * @return				IMenuPanel object.
		 */
		virtual IMenuPanel *CreatePanel() =0;

		/**
		 * @brief Creates an IBaseMenu object of this style.
		 *
		 * Note: the object should be freed using IBaseMenu::Destroy.
		 *
		 * @param handler		IMenuHandler pointer.
		 * @param pOwner		Optional IdentityToken_t owner for handle 
		 *						creation.
		 * @return				An IBaseMenu pointer.
		 */
		virtual IBaseMenu *CreateMenu(IMenuHandler *handler, IdentityToken_t *pOwner=NULL) =0;

		/**
		 * @brief Returns the maximum number of items per page.
		 *
		 * Menu implementations must return >= 2.  Styles with only 1 or 0 
		 * items per page are not valid.
		 *
		 * @return				Number of items per page.
		 */
		virtual unsigned int GetMaxPageItems() =0;

		/**
		 * @brief Returns whether or not a client is viewing a menu.
		 *
		 * @param client		Client index.
		 * @param object		Optional pointer to retrieve menu object,
		 *						if any.
		 * @return				MenuSource value.
		 */
		virtual MenuSource GetClientMenu(int client, void **object) =0;

		/**
		 * @brief Cancels a client's menu.
		 *
		 * @param client		Client index.
		 * @param autoIgnore	If true, no menus can be created during 
		 *						the cancellation process.
		 * @return				True if a menu was cancelled, false otherwise.
		 */
		virtual bool CancelClientMenu(int client, bool autoIgnore=false) =0;

		/**
		 * @brief Returns a Handle the IMenuStyle object.
		 *
		 * @return				Handle_t pointing to this object.
		 */
		virtual Handle_t GetHandle() =0;

		/**
		 * @brief For the Handle system, returns approximate memory usage.
		 *
		 * @return				Approximate number of bytes being used.
		 */
		virtual unsigned int GetApproxMemUsage() =0;
		
		/**
		 * @brief Returns whether or not this style is supported by the current game.
		 *
		 * @return				True if supported, otherwise false.
		 */
		virtual bool IsSupported() =0;
	};

	/**
	 * @brief High-level interface for building menus.
	 */
	class IBaseMenu
	{
	public:
		/**
		 * @brief Appends an item to the end of a menu.
		 *
		 * @param info			Item information string.
		 * @param draw			Default drawing information.
		 * @return				True on success, false on item limit reached.
		 */
		virtual bool AppendItem(const char *info, const ItemDrawInfo &draw) =0;

		/**
		 * @brief Inserts an item into the menu before a certain position; 
		 * the new item will be at the given position and all next items 
		 * pushed forward.
		 *
		 * @param position		Position, starting from 0.
		 * @param info			Item information string.
		 * @param draw			Default item draw info.
		 * @return				True on success, false on invalid menu position
		 */
		virtual bool InsertItem(unsigned int position, const char *info, const ItemDrawInfo &draw) =0;

		/**
		 * @brief Removes an item from the menu.
		 *
		 * @param position		Position, starting from 0.
		 * @return				True on success, false on invalid menu position.
		 */
		virtual bool RemoveItem(unsigned int position) =0;

		/**
		 * @brief Removes all items from the menu.
		 */
		virtual void RemoveAllItems() =0;

		/**
		 * @brief Returns an item's info.
		 *
		 * @param position		Position, starting from 0.
		 * @param draw			Optional pointer to store a draw information.
		 * @param client		Client index. (Important for randomized menus.)
		 * @return				Info string pointer, or NULL if position was invalid.
		 */
		virtual const char *GetItemInfo(unsigned int position, ItemDrawInfo *draw, int client=0) =0;

		/**
		 * @brief Returns the number of items.
		 *
		 * @return				Number of items in the menu.
		 */
		virtual unsigned int GetItemCount() =0;

		/** 
		 * @brief Sets the menu's pagination,.
		 *
		 * If pagination is set to MENU_NO_PAGINATION, and the previous 
		 * pagination was not MENU_NO_PAGINATION, then the MENUFLAG_BUTTON_EXIT 
		 * is unset.  It can be re-applied if desired.
		 *
		 * @param itemsPerPage	Number of items per page, or MENU_NO_PAGINATION.
		 * @return				True on success, false if itemsPerPage is too 
		 *						large.
		 */
		virtual bool SetPagination(unsigned int itemsPerPage) =0;

		/**
		 * @brief Returns an item's pagination.
		 *
		 * @return				Pagination setting.
		 */
		virtual unsigned int GetPagination() =0;

		/**
		 * @brief Returns the menu style.
		 * 
		 * @return				Menu style.
		 */
		virtual IMenuStyle *GetDrawStyle() =0;

		/**
		 * @brief Sets the menu's display title/message.
		 *
		 * @param message		Message (format options allowed).
		 */
		virtual void SetDefaultTitle(const char *message) =0;

		/**
		 * @brief Returns the menu's display/title message.
		 *
		 * @return				Message string.
		 */
		virtual const char *GetDefaultTitle() =0;

		/**
		 * @brief Sets an extended menu option.
		 *
		 * @param option		Option type.
		 * @param valuePtr		Pointer of the type expected by the option.
		 * @return				True on success, false if option or value is not supported.
		 */
		virtual bool SetExtOption(MenuOption option, const void *valuePtr) =0;

		/**
		 * @brief Creates a new IMenuPanel object using extended options specific
		 * to the IMenuStyle parent.  Titles, items, etc, are not copied.
		 *
		 * Note: The object should be freed with IMenuPanel::DeleteThis.
		 *
		 * @return				IMenuPanel pointer.
		 */
		virtual IMenuPanel *CreatePanel() =0;

		/**
		 * @brief Sends the menu to a client.
		 *
		 * @param client		Client index to display to.
		 * @param time			Time to hold menu for.
		 * @param alt_handler	Alternate IMenuHandler.
		 * @return				True on success, false otherwise.
		 */
		virtual bool Display(int client, unsigned int time, IMenuHandler *alt_handler=NULL) =0;

		/**
		 * @brief Destroys the menu and frees all associated resources.
		 *
		 * @param releaseHandle	If true, the Handle will be released
		 *						in the destructor.  This should be set
		 *						to true except for IHandleTypeDispatch
		 *						destructors.
		 */
		virtual void Destroy(bool releaseHandle=true) =0;

		/**
		 * @brief Cancels the menu on all client's displays.  While the menu is
		 * being cancelled, the menu may not be re-displayed to any clients.
		 * If a vote menu is currently active, it will be cancelled as well.
		 *
		 * @return				Number of menus cancelled.
		 */
		virtual void Cancel() =0;

		/**
		 * @brief Returns the menu's Handle.  The Handle is automatically
		 * removed when the menu is destroyed.
		 *
		 * @return				Handle_t handle value.
		 */
		virtual Handle_t GetHandle() =0;
		
		/**
		 * @brief Returns menu option flags.
		 *
		 * @return				Menu option flags.
		 */
		virtual unsigned int GetMenuOptionFlags() =0;

		/**
		 * @brief Sets menu option flags.
		 *
		 * @param flags			Menu option flags.
		 */
		virtual void SetMenuOptionFlags(unsigned int flags) =0;

		/**
		 * @brief Returns the menu's handler.
		 *
		 * @return				IMenuHandler of the menu.
		 */
		virtual IMenuHandler *GetHandler() =0;

		/**
		 * @brief Sends the menu to a client, starting from the given item number. 
		 *
		 * Note: this API call was added in v13.
		 *
		 * @param client		Client index to display to.
		 * @param time			Time to hold menu for.
		 * @param start_item	Starting item to draw.
		 * @param alt_handler	Alternate IMenuHandler.
		 * @return				True on success, false otherwise.
		 */
		virtual bool DisplayAtItem(int client,
			unsigned int time,
			unsigned int start_item,
			IMenuHandler *alt_handler=NULL) =0;

		/**
		 * @brief For the Handle system, returns approximate memory usage.
		 *
		 * @return				Approximate number of bytes being used.
		 */
		virtual unsigned int GetApproxMemUsage() =0;

		/**
		 * @brief Generates a per-client random mapping for the current vote options.
		 * @param start			Menu item index to start randomizing from.
		 * @param stop			Menu item index to stop randomizing at. -1 = infinite
		 */
		virtual void ShufflePerClient(int start=0, int stop=-1) =0;

		/**
		 * @brief Fills the client vote option mapping with user supplied values.
		 * @param client		Client index.
		 * @param array			Integer array with mapping.
		 * @param length		Length of array.
		 */
		virtual void SetClientMapping(int client, int *array, int length) =0;

		/**
		 * @brief Returns true if the menu has a per-client random mapping.
		 * @return				True on success, false otherwise.
		 */
		virtual bool IsPerClientShuffled() =0;

		/**
		 * @brief Returns the actual (not shuffled) item index from the client position.
		 * @param client		Client index.
		 * @param position		Position, starting from 0.
		 * @return				Actual item index in menu.
		 */
		virtual unsigned int GetRealItemIndex(int client, unsigned int position) =0;
	};

	/** 
	 * @brief Contains callbacks for menu actions.
	 */
	class IMenuHandler
	{
	public:
		/**
		 * @brief Returns the menu api verison.
		 *
		 * @return				Menu API version.
		 */
		virtual unsigned int GetMenuAPIVersion2()
		{
			return SMINTERFACE_MENUMANAGER_VERSION;
		}

		/** 
		 * @brief A display/selection cycle has started.
		 *
		 * @param menu			Menu pointer.
		 */
		virtual void OnMenuStart(IBaseMenu *menu)
		{
		}

		/**
		 * @brief Called before a menu is being displayed.  This is where
		 * you can set an alternate title on the menu.
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client index.
		 * @param display		IMenuPanel pointer.
		 */
		virtual void OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display)
		{
		}

		/**
		 * @brief Called when an item is selected.
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client that selected the item.
		 * @param item			Item number.
		 */
		virtual void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
		{
		}

		/**
		 * @brief An active menu display was dropped from a client.
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client that had the menu.
		 * @param reason		Menu cancellation reason.
		 */
		virtual void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
		{
		}

		/**
		 * @brief A display/selection cycle has ended.
		 *
		 * @param menu			Menu pointer.
		 * @param reason		MenuEndReason reason.
		 */
		virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
		}

		/**
		 * @brief Called when the menu object is destroyed.
		 *
		 * @param menu			Menu pointer.
		 */
		virtual void OnMenuDestroy(IBaseMenu *menu)
		{
		}

		/**
		 * @brief Called when requesting how to render an item.
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client index receiving the menu.
		 * @param item			Item number in the menu.
		 * @param style			ITEMSTYLE flags, by reference for modification.
		 */
		virtual void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style)
		{
		}

		/**
		 * @brief Called when drawing item text.
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client index receiving the menu.
		 * @param panel			Panel being used to draw the menu.
		 * @param item			Item number in the menu.
		 * @param dr			Item draw information.
		 * @return				0 to let the render algorithm decide how to draw, otherwise,
		 *						the return value from panel->DrawItem should be returned.
		 */
		virtual unsigned int OnMenuDisplayItem(IBaseMenu *menu, 
											   int client, 
											   IMenuPanel *panel,
											   unsigned int item, 
											   const ItemDrawInfo &dr)
		{
			return 0;
		}

		/**
		 * @brief Called when a vote has been started and displayed to 
		 * clients.  This is called after OnMenuStart() and OnMenuDisplay(),
		 * but before OnMenuSelect().
		 *
		 * @param menu			Menu pointer.
		 */
		virtual void OnMenuVoteStart(IBaseMenu *menu)
		{
		}

		/**
		 * @brief Called when a vote ends.  This is automatically called by the 
		 * wrapper, and never needs to called from a style implementation.  
		 *
		 * This function does not replace OnMenuEnd(), nor does it have the 
		 * same meaning as OnMenuEnd(), meaning you should not destroy a menu
		 * while it is in this function.
		 *
		 * @param menu			Menu pointer.
		 * @param results		Menu vote results.
		 */
		virtual void OnMenuVoteResults(IBaseMenu *menu, const menu_vote_result_t *results)
		{
		}

		/**
		 * @brief Called when a vote is cancelled.  If this is called, then 
		 * OnMenuVoteResults() will not be called.  In both cases, OnMenuEnd will 
		 * always be called.
		 *
		 * @param menu			Menu pointer.
		 * @param reason		VoteCancelReason reason.
		 */
		virtual void OnMenuVoteCancel(IBaseMenu *menu, VoteCancelReason reason)
		{
		}

		/**
		 * @brief Call to set private handler stuff.
		 *
		 * @param option		Option name.
		 * @param data			Private data.
		 * @return				True if set, false if invalid or unrecognized.
		 */
		virtual bool OnSetHandlerOption(const char *option, const void *data)
		{
			return false;
		}

		/**
		 * @brief Called when an item is selected.
		 *
		 * Note: This callback was added in v13.  It is called after OnMenuSelect().
		 *
		 * @param menu			Menu pointer.
		 * @param client		Client that selected the item.
		 * @param item			Item number.
		 * @param item_on_page	The first item on the page the player was last 
		 *						viewing.
		 */
		virtual void OnMenuSelect2(IBaseMenu *menu,
			int client,
			unsigned int item,
			unsigned int item_on_page)
		{
		}
	};

	/**
	 * @brief Manages menu creation and displaying.
	 */
	class IMenuManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_MENUMANAGER_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_MENUMANAGER_VERSION;
		}
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version < 11 || version > GetInterfaceVersion())
			{
				return false;
			}
			return true;
		}
	public:
		/**
		 * @brief Finds a style by name.
		 *
		 * @param name			Name of the style (case insensitive).
		 * @return				IMenuStyle pointer, or NULL if not found.
		 */
		virtual IMenuStyle *FindStyleByName(const char *name) =0;

		/**
		 * @brief Returns the default draw style Core is using.
		 *
		 * @return				Menu style pointer.
		 */
		virtual IMenuStyle *GetDefaultStyle() =0;

		/**
		 * @brief Given a set of menu states, converts it to an IMenuPanel object.
		 *
		 * The state parameter is both INPUT and OUTPUT.
		 * INPUT: menu, mh, firstItem, lastItem
		 * OUTPUT: display, firstItem, lastItem, slots
		 *
		 * @param client		Client index.
		 * @param states		Menu states.
		 * @param order			Order to search for items.
		 * @return				IMenuPanel pointer, or NULL if no items could be 
		 *						found in the IBaseMenu pointer, or NULL if any
		 *						other error occurred.  Any valid pointer must
		 *						be freed using IMenuPanel::DeleteThis.
		 */
		virtual IMenuPanel *RenderMenu(int client, menu_states_t &states, ItemOrder order) =0;

		/**
		 * @brief Cancels a menu.  Calls IBaseMenu::Cancel() after doing some preparatory 
		 * work.  This should always be used instead of directly calling Cancel().
		 *
		 * @param menu			IBaseMenu pointer.
		 */
		virtual void CancelMenu(IBaseMenu *menu) =0;

		/**
		 * @brief Displays a menu as a vote.
		 *
		 * @param menu			IBaseMenu pointer.
		 * @param num_clients	Number of clients to display to.
		 * @param clients		Client index array.
		 * @param max_time		Maximum time to hold menu for.
		 * @param flags			Vote flags.
		 * @return				True on success, false if a vote is in progress.
		 */
		virtual bool StartVote(IBaseMenu *menu,
								unsigned int num_clients,
								int clients[],
								unsigned int max_time,
								unsigned int flags=0) =0;

		/**
		 * @brief Returns whether or not a vote is in progress.
		 *
		 * @return				True if a vote is in progress, false otherwise.
		 */
		virtual bool IsVoteInProgress() =0;

		/**
		 * @brief Cancels the vote in progress.  This calls IBaseMenu::Cancel().
		 */
		virtual void CancelVoting() =0;

		/**
		 * @brief Returns the remaining vote delay from the last menu.  This delay is 
		 * a suggestion for all public votes, and is not enforced.
		 *
		 * @return				Number of seconds to wait.
		 */
		virtual unsigned int GetRemainingVoteDelay() =0;

		/**
		 * @brief Returns whether a client is in the "allowed to vote" pool determined 
		 * by the initial call to StartVote().
		 *
		 * @param client		Client index.
		 * @return				True if client is allowed to vote, false on failure.
		 */
		virtual bool IsClientInVotePool(int client) =0;

		/**
		 * @brief Redraws the current vote menu to a client in the voting pool.
		 *
		 * @param client		Client index.
		 * @return				True on success, false if client is not allowed to vote.
		 */
		virtual bool RedrawClientVoteMenu(int client) =0;

		/**
		 * @brief Redraws the current vote menu to a client in the voting pool.
		 *
		 * @param client		Client index.
		 * @param revotes		True to allow revotes, false otherwise.
		 * @return				True on success, false if client is not allowed to vote.
		 */
		virtual bool RedrawClientVoteMenu2(int client, bool revotes) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_MENU_SYSTEM_H_

