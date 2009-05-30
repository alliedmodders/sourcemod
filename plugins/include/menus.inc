/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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
 
#if defined _menus_included
 #endinput
#endif
#define _menus_included

/**
 * Low-level drawing style of the menu.
 */
enum MenuStyle
{
	MenuStyle_Default = 0,		/**< The "default" menu style for the mod */
	MenuStyle_Valve = 1,		/**< The Valve provided menu style (Used on HL2DM) */
	MenuStyle_Radio = 2,		/**< The simpler menu style commonly used on CS:S */
};

/**
 * Different actions for the menu "pump" callback
 */
enum MenuAction
{
	MenuAction_Start = (1<<0),		/**< A menu has been started (nothing passed) */
	MenuAction_Display = (1<<1),	/**< A menu is about to be displayed (param1=client, param2=MenuPanel Handle) */
	MenuAction_Select = (1<<2),		/**< An item was selected (param1=client, param2=item) */
	MenuAction_Cancel = (1<<3),		/**< The menu was cancelled (param1=client, param2=reason) */
	MenuAction_End = (1<<4),		/**< A menu display has fully ended.
										 param1 is the MenuEnd reason, and if it's MenuEnd_Cancelled, then 
										 param2 is the MenuCancel reason from MenuAction_Cancel.
										 */
	MenuAction_VoteEnd = (1<<5),	/**< (VOTE ONLY): A vote sequence has succeeded (param1=chosen item) 
										 This is not called if SetVoteResultCallback has been used on the menu. */
	MenuAction_VoteStart = (1<<6), 	/**< (VOTE ONLY): A vote sequence has started (nothing passed) */
	MenuAction_VoteCancel = (1<<7),	/**< (VOTE ONLY): A vote sequence has been cancelled (param1=reason) */
	MenuAction_DrawItem = (1<<8),	/**< An item is being drawn; return the new style (param1=client, param2=item) */
	MenuAction_DisplayItem = (1<<9),/**< Item text is being drawn to the display (param1=client, param2=item)
										  To change the text, use RedrawMenuItem().
										  If you do so, return its return value.  Otherwise, return 0.
										  */
};

/** Default menu actions */
#define MENU_ACTIONS_DEFAULT	MenuAction_Select|MenuAction_Cancel|MenuAction_End
/** All menu actions */
#define MENU_ACTIONS_ALL		MenuAction:0xFFFFFFFF

#define MENU_NO_PAGINATION		0		/**< Menu should not be paginated (10 items max) */
#define MENU_TIME_FOREVER		0		/**< Menu should be displayed as long as possible */

#define ITEMDRAW_DEFAULT		(0)		/**< Item should be drawn normally */
#define ITEMDRAW_DISABLED		(1<<0)	/**< Item is drawn but not selectable */
#define ITEMDRAW_RAWLINE		(1<<1)	/**< Item should be a raw line, without a slot */
#define ITEMDRAW_NOTEXT			(1<<2)	/**< No text should be drawn */
#define ITEMDRAW_SPACER			(1<<3)	/**< Item should be drawn as a spacer, if possible */
#define ITEMDRAW_IGNORE	((1<<1)|(1<<2))	/**< Item should be completely ignored (rawline + notext) */
#define ITEMDRAW_CONTROL		(1<<4)	/**< Item is control text (back/next/exit) */

#define MENUFLAG_BUTTON_EXIT		(1<<0)	/**< Menu has an "exit" button (default if paginated) */
#define MENUFLAG_BUTTON_EXITBACK	(1<<1)	/**< Menu has an "exit back" button */
#define MENUFLAG_NO_SOUND			(1<<2)	/**< Menu will not have any select sounds */

#define VOTEINFO_CLIENT_INDEX		0		/**< Client index */
#define VOTEINFO_CLIENT_ITEM		1		/**< Item the client selected, or -1 for none */
#define VOTEINFO_ITEM_INDEX			0		/**< Item index */
#define VOTEINFO_ITEM_VOTES			1		/**< Number of votes for the item */

#define VOTEFLAG_NO_REVOTES			(1<<0)	/**< Players cannot change their votes */

/**
 * Reasons a menu can be cancelled (MenuAction_Cancel).
 */
enum
{
	MenuCancel_Disconnected = -1,	/**< Client dropped from the server */
	MenuCancel_Interrupted = -2,	/**< Client was interrupted with another menu */
	MenuCancel_Exit = -3,			/**< Client exited via "exit" */
	MenuCancel_NoDisplay = -4,		/**< Menu could not be displayed to the client */
	MenuCancel_Timeout = -5,		/**< Menu timed out */
	MenuCancel_ExitBack = -6,		/**< Client selected "exit back" on a paginated menu */
};

/**
 * Reasons a vote can be cancelled (MenuAction_VoteCancel).
 */
enum
{
	VoteCancel_Generic = -1,		/**< Vote was generically cancelled. */
	VoteCancel_NoVotes = -2,		/**< Vote did not receive any votes. */
};

/**
 * Reasons a menu ended (MenuAction_End).
 */
enum
{
	MenuEnd_Selected = 0,			/**< Menu item was selected */
	MenuEnd_VotingDone = -1,		/**< Voting finished */
	MenuEnd_VotingCancelled = -2,	/**< Voting was cancelled */
	MenuEnd_Cancelled = -3,			/**< Menu was cancelled (reason in param2) */
	MenuEnd_Exit = -4,				/**< Menu was cleanly exited via "exit" */
	MenuEnd_ExitBack = -5,			/**< Menu was cleanly exited via "back" */
};

/**
 * Describes a menu's source
 */
enum MenuSource
{
	MenuSource_None = 0,			/**< No menu is being displayed */	
	MenuSource_External = 1,		/**< External menu */
	MenuSource_Normal = 2,			/**< A basic menu is being displayed */
	MenuSource_RawPanel = 3,		/**< A display is active, but it is not tied to a menu */
};

/**
 * Called when a menu action is completed.
 *
 * @param menu				The menu being acted upon.
 * @param action			The action of the menu.
 * @param param1			First action parameter (usually the client).
 * @param param2			Second action parameter (usually the item).
 * @noreturn
 */
functag public MenuHandler(Handle:menu, MenuAction:action, param1, param2);

/**
 * Creates a new, empty menu using the default style.
 * 
 * @param handler			Function which will receive menu actions.
 * @param actions			Optionally set which actions to receive.  Select, 
 *							Cancel, and End will always be received regardless
 *							of whether they are set or not.  They are also
 *							the only default actions.
 * @return					A new menu Handle.
 */
native Handle:CreateMenu(MenuHandler:handler, MenuAction:actions=MENU_ACTIONS_DEFAULT);

/**
 * Displays a menu to a client.
 *
 * @param menu				Menu Handle.
 * @param client			Client index.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or client not in game.
 */
native bool:DisplayMenu(Handle:menu, client, time);

/**
 * Displays a menu to a client, starting from the given item.
 *
 * @param menu				Menu Handle.
 * @param client			Client index.
 * @param first_item		First item to begin drawing from.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or client not in game.
 */
native bool:DisplayMenuAtItem(Handle:menu, client, first_item, time);

/**
 * Appends a new item to the end of a menu.
 *
 * @param menu				Menu Handle.
 * @param info				Item information string.
 * @param display			Default item display string.
 * @param style				Drawing style flags.  Anything other than DEFAULT or 
 *							DISABLED will be completely ignored when paginating.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or item limit reached.
 */
native AddMenuItem(Handle:menu, 
					const String:info[], 
					const String:display[], 
					style=ITEMDRAW_DEFAULT);

/**
 * Inserts an item into the menu before a certain position; the new item will
 * be at the given position and all next items pushed forward.
 *
 * @param menu				Menu Handle.
 * @param position			Position, starting from 0.
 * @param info				Item information string.
 * @param display			Default item display string.
 * @param style				Drawing style flags.  Anything other than DEFAULT or 
 *							DISABLED will be completely ignored when paginating.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or menu position.
 */
native bool:InsertMenuItem(Handle:menu,
						position, 
						const String:info[], 
						const String:display[], 
						style=ITEMDRAW_DEFAULT);

/**
 * Removes an item from the menu.
 *
 * @param menu				Menu Handle.
 * @param position			Position, starting from 0.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or menu position.
 */
native bool:RemoveMenuItem(Handle:menu, position);

/**
 * Removes all items from a menu.
 *
 * @param menu				Menu Handle.
 * @noreturn
 * @error					Invalid Handle or menu position.
 */
native RemoveAllMenuItems(Handle:menu);

/**
 * Retrieves information about a menu item.
 *
 * @param menu				Menu Handle.
 * @param position			Position, starting from 0.
 * @param infoBuf			Info buffer.
 * @param infoBufLen		Maximum length of the info buffer.
 * @param style				By-reference variable to store drawing flags.
 * @param dispBuf			Display buffer.
 * @param dispBufLen		Maximum length of the display buffer.
 * @return					True on success, false if position is invalid.
 * @error					Invalid Handle.
 */
native bool:GetMenuItem(Handle:menu, 
						position, 
						String:infoBuf[], 
						infoBufLen, 
						&style=0, 
						String:dispBuf[]="",
						dispBufLen=0);
						
/**
 * Returns the first item on the page of a currently selected menu.
 *
 * This is only valid inside a MenuAction_Select callback.
 *
 * @return					First item number on the page the client was viewing 
 *							before selecting the item in the callback.  This can 
 *							be used to re-display the menu from the original 
 *							position.
 * @error					Not called from inside a MenuAction_Select callback.
 */
native GetMenuSelectionPosition();

/**
 * Returns the number of items in a menu.
 *
 * @param menu				Menu Handle.
 * @return					Number of items in the menu.
 * @error					Invalid Handle.
 */
native GetMenuItemCount(Handle:menu);

/**
 * Sets whether the menu should be paginated or not.
 *
 * If itemsPerPage is MENU_NO_PAGINATION, and the exit button flag is set, 
 * then the exit button flag is removed.  It can be re-applied if desired.
 *
 * @param menu				Handle to the menu.
 * @param itemsPerPage		Number of items per page, or MENU_NO_PAGINATION.
 * @return					True on success, false if pagination is too high or 
 *							low.
 * @error					Invalid Handle.
 */
native bool:SetMenuPagination(Handle:menu, itemsPerPage);

/**
 * Returns a menu's pagination setting.
 *
 * @param menu				Handle to the menu.
 * @return					Pagination setting.
 * @error					Invalid Handle.
 */
native GetMenuPagination(Handle:menu);

/**
 * Returns a menu's MenuStyle Handle.  The Handle
 * is global and cannot be freed.
 *
 * @param menu				Handle to the menu.
 * @return 					Handle to the menu's draw style.
 * @error					Invalid Handle.
 */
native Handle:GetMenuStyle(Handle:menu);

/**
 * Sets the menu's default title/instruction message.
 *
 * @param menu				Menu Handle.
 * @param fmt				Message string format
 * @param ...				Message string arguments.
 * @noreturn
 * @error					Invalid Handle.
 */
native SetMenuTitle(Handle:menu, const String:fmt[], any:...);

/**
 * Returns the text of a menu's title.
 *
 * @param menu				Menu Handle.
 * @param buffer			Buffer to store title.
 * @param maxlength			Maximum length of the buffer.
 * @return					Number of bytes written.
 * @error					Invalid Handle/
 */
native GetMenuTitle(Handle:menu, String:buffer[], maxlength);

/**
 * Creates a raw MenuPanel based off the menu's style.
 * The Handle must be freed with CloseHandle().
 *
 * @return					A new MenuPanel Handle.
 * @error					Invalid Handle.
 */
native Handle:CreatePanelFromMenu(Handle:menu);

/**
 * Returns whether or not the menu has an exit button.
 * By default, menus have an exit button.
 *
 * @param menu				Menu Handle.
 * @return 					True if the menu has an exit button; false otherwise.
 * @error					Invalid Handle.
 */
native bool:GetMenuExitButton(Handle:menu);

/**
 * Sets whether or not the menu has an exit button.  By default, paginated menus 
 * have an exit button.
 *
 * If a menu's pagination is changed to MENU_NO_PAGINATION, and the pagination 
 * was previously a different value, then the Exit button status is changed to 
 * false.  It must be explicitly re-enabled afterwards.
 *
 * If a non-paginated menu has an exit button, then at most 9 items will be 
 * displayed.
 *
 * @param menu				Menu Handle.
 * @param button			True to enable the button, false to remove it.
 * @return 					True if allowed; false on failure.
 * @error					Invalid Handle.
 */
native bool:SetMenuExitButton(Handle:menu, bool:button);

/**
 * Returns whether or not the menu has an "exit back" button.  By default, 
 * menus do not have an exit back button.  
 *
 * Exit Back buttons appear as "Back" on page 1 of paginated menus and have 
 * functionality defined by the user in MenuEnd_ExitBack.
 *
 * @param menu				Menu Handle.
 * @return 					True if the menu has an exit back button; false otherwise.
 * @error					Invalid Handle.
 */
native bool:GetMenuExitBackButton(Handle:menu);

/**
 * Sets whether or not the menu has an "exit back" button. By default, menus 
 * do not have an exit back button.
 *
 * Exit Back buttons appear as "Back" on page 1 of paginated menus and have 
 * functionality defined by the user in MenuEnd_ExitBack.
 *
 * @param menu				Menu Handle.
 * @param button			True to enable the button, false to remove it.
 * @error					Invalid Handle.
 */
native SetMenuExitBackButton(Handle:menu, bool:button);


/**
 * Cancels a menu from displaying on all clients.  While the
 * cancellation is in progress, this menu cannot be re-displayed
 * to any clients.
 *
 * The menu may still exist on the client's screen after this command.
 * This simply verifies that the menu is not being used anywhere.
 *
 * If any vote is in progress on a menu, it will be cancelled.
 *
 * @param menu				Menu Handle.
 * @noreturn
 * @error					Invalid Handle.
 */
native CancelMenu(Handle:menu);

/**
 * Retrieves a menu's option flags.
 *
 * @param menu				Menu Handle.
 * @return					A bitstring of MENUFLAG bits.
 * @error					Invalid Handle.
 */
native GetMenuOptionFlags(Handle:menu);

/**
 * Sets a menu's option flags.
 *
 * If a certain bit is not supported, it will be stripped before being set.
 * See SetMenuExitButton() for information on Exit buttons.
 * See SetMenuExitBackButton() for information on Exit Back buttons.
 *
 * @param menu				Menu Handle.
 * @param flags				A new bitstring of MENUFLAG bits.
 * @noreturn
 * @error					Invalid Handle.
 */
native SetMenuOptionFlags(Handle:menu, flags);

/**
 * Returns whether a vote is in progress.
 *
 * @param menu				Deprecated; no longer used.
 * @return					True if a vote is in progress, false otherwise.
 */
native bool:IsVoteInProgress(Handle:menu=INVALID_HANDLE);

/**
 * Cancels the vote in progress.
 *
 * @noreturn
 * @error					If no vote is in progress.
 */
native CancelVote();

/**
 * Broadcasts a menu to a list of clients.  The most selected item will be 
 * returned through MenuAction_End.  On a tie, a random item will be returned 
 * from a list of the tied items.
 *
 * Note that MenuAction_VoteEnd and MenuAction_VoteStart are both
 * default callbacks and do not need to be enabled.
 *
 * @param menu				Menu Handle.
 * @param clients			Array of clients to broadcast to.
 * @param numClients		Number of clients in the array.
 * @param time				Maximum time to leave menu on the screen.
 * @param flags				Optional voting flags.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle, or a vote is already in progress.
 */
native bool:VoteMenu(Handle:menu, clients[], numClients, time, flags=0);

/**
 * Sends a vote menu to all clients.  See VoteMenu() for more information.
 *
 * @param menu				Menu Handle.
 * @param time				Maximum time to leave menu on the screen.
 * @param flags				Optional voting flags.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle.
 */
stock VoteMenuToAll(Handle:menu, time, flags=0)
{
	new num = GetMaxClients();
	new total;
	decl players[num];
	
	for (new i=1; i<=num; i++)
	{
		if (!IsClientInGame(i))
		{
			continue;
		}
		players[total++] = i;
	}
	
	return VoteMenu(menu, players, total, time, flags);
}
/**
 * Callback for when a vote has ended and results are available.
 *
 * @param menu				The menu being voted on.
 * @param num_votes			Number of votes tallied in total.
 * @param num_clients		Number of clients who could vote.
 * @param client_info		Array of clients.  Use VOTEINFO_CLIENT_ defines.
 * @param num_items			Number of unique items that were selected.
 * @param item_info			Array of items, sorted by count.  Use VOTEINFO_ITEM
 *							defines.
 * @noreturn
 */
functag public VoteHandler(Handle:menu,
						   num_votes, 
						   num_clients,
						   const client_info[][2], 
						   num_items,
						   const item_info[][2]);	   

/**
 * Sets an advanced vote handling callback.  If this callback is set,
 * MenuAction_VoteEnd will not be called.
 *
 * @param menu				Menu Handle.
 * @param callback			Callback function.
 * @noreturn
 * @error					Invalid Handle or callback.
 */
native SetVoteResultCallback(Handle:menu, VoteHandler:callback);

/**
 * Returns the number of seconds you should "wait" before displaying
 * a publicly invocable menu.  This number is the time remaining until
 * (last_vote + sm_vote_delay).
 *
 * @return					Number of seconds to wait, or 0 for none.
 */
native CheckVoteDelay();

/**
 * Returns whether a client is in the pool of clients allowed 
 * to participate in the current vote.  This is determined by 
 * the client list passed to VoteMenu().
 *
 * @param client			Client index.
 * @return					True if client is allowed to vote, false otherwise.
 * @error					If no vote is in progress or client index is invalid.
 */
native bool:IsClientInVotePool(client);

/**
 * Redraws the current vote menu to a client in the voting pool.
 *
 * @param client			Client index.
 * @param revotes			True to allow revotes, false otherwise.
 * @return					True on success, false if the client is in the vote pool 
 *							but cannot vote again.
 * @error					No vote in progress, client is not in the voting pool, 
 *							or client index is invalid.
 */
native bool:RedrawClientVoteMenu(client, bool:revotes=true);

/**
 * Returns a style's global Handle.
 *
 * @param style			Menu Style.
 * @return				A Handle, or INVALID_HANDLE if not found or unusable.
 */
native Handle:GetMenuStyleHandle(MenuStyle:style);

/**
 * Creates a MenuPanel from a MenuStyle.  Panels are used for drawing raw 
 * menus without any extra helper functions.  The Handle must be closed
 * with CloseHandle().
 *
 * @param hStyle		MenuStyle Handle, or INVALID_HANDLE to use the default style.
 * @return				A new MenuPanel Handle.
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native Handle:CreatePanel(Handle:hStyle=INVALID_HANDLE);

/** 
 * Creates a Menu from a MenuStyle.  The Handle must be closed with 
 * CloseHandle().
 *
 * @parma hStyle		MenuStyle Handle, or INVALID_HANDLE to use the default style.
 * @param handler		Function which will receive menu actions.
 * @param actions		Optionally set which actions to receive.  Select, 
 *						Cancel, and End will always be received regardless
 *						of whether they are set or not.  They are also
 *						the only default actions.
 * @return				A new menu Handle.
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native Handle:CreateMenuEx(Handle:hStyle=INVALID_HANDLE, MenuHandler:handler, MenuAction:actions=MENU_ACTIONS_DEFAULT);

/**
 * Returns whether a client is viewing a menu.
 *
 * @param client		Client index.
 * @param hStyle		MenuStyle Handle, or INVALID_HANDLE to use the default style.
 * @return				A MenuSource value.
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native MenuSource:GetClientMenu(client, Handle:hStyle=INVALID_HANDLE);

/** 
 * Cancels a menu on a client.  This will only affect non-external menus.
 *
 * @param hstyle		MenuStyle Handle, or INVALID_HANDLE to use the default style.
 * @param client		Client index.
 * @param autoIgnore	If true, no menus can be re-drawn on the client during
 *						the cancellation process.
 * @return				True if a menu was cancelled, false otherwise.
 */
native bool:CancelClientMenu(client, bool:autoIgnore=false, Handle:hStyle=INVALID_HANDLE);

/**
 * Returns a style's maximum items per page.
 *
 * @param hStyle		MenuStyle Handle, or INVALID_HANDLE to use the default style.
 * @return				Maximum items per page.
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native GetMaxPageItems(Handle:hStyle=INVALID_HANDLE);

/**
 * Returns a MenuPanel's parent style.
 *
 * @param panel			A MenuPanel Handle.
 * @return				The MenuStyle Handle that created the panel.
 * @error				Invalid Handle.
 */
native Handle:GetPanelStyle(Handle:panel);

/**
 * Sets the panel's title.
 *
 * @param panel			A MenuPanel Handle.
 * @param title			Text to set as the title.
 * @param onlyIfEmpty	If true, the title will only be set if no title is set.
 * @noreturn
 * @error				Invalid Handle.
 */
native Handle:SetPanelTitle(Handle:panel, const String:text[], bool:onlyIfEmpty=false);

/**
 * Draws an item on a panel.  If the item takes up a slot, the position
 * is returned.
 *
 * @param panel			A MenuPanel Handle.
 * @param text			Display text to use.  If not a raw line, 
 *						the style may automatically add color markup.
 *						No numbering or newlines are needed.
 * @param style			ITEMDRAW style flags.
 * @return				A slot position, or 0 if item was a rawline or could not be drawn.
 * @error				Invalid Handle.
 */
native DrawPanelItem(Handle:panel, const String:text[], style=ITEMDRAW_DEFAULT);

/**
 * Draws a raw line of text on a panel, without any markup other than a newline.
 *
 * @param panel			A MenuPanel Handle, or INVALID_HANDLE if inside a 
 *						MenuAction_DisplayItem callback.
 * @param text			Display text to use.
 * @return				True on success, false if raw lines are not supported.
 * @error				Invalid Handle.
 */
native DrawPanelText(Handle:panel, const String:text[]);

/**
 * Returns whether or not the given drawing flags are supported by
 * the menu style.
 *
 * @param panel			A MenuPanel Handle.
 * @param style			ITEMDRAW style flags.
 * @return				True if item is drawable, false otherwise.
 * @error				Invalid Handle.
 */
native CanPanelDrawFlags(Handle:panel, style);

/**
 * Sets the selectable key map of a panel.  This is not supported by
 * all styles (only by Radio, as of this writing).  
 *
 * @param keys			An integer where each bit N allows key
 *						N+1 to be selected.  If no keys are selectable,
 *						then key 0 (bit 9) is automatically set.
 * @return 				True if supported, false otherwise.
 */
native bool:SetPanelKeys(Handle:panel, keys);

/**
 * Sends a panel to a client.  Unlike full menus, the handler
 * function will only receive the following actions, both of 
 * which will have INVALID_HANDLE for a menu, and the client
 * as param1.
 *
 * MenuAction_Select (param2 will be the key pressed)
 * MenuAction_Cancel (param2 will be the reason)
 *
 * Also, if the menu fails to display, no callbacks will be called.
 *
 * @param panel			A MenuPanel Handle.
 * @param client		A client to draw to.
 * @param handler		The MenuHandler function to catch actions with.
 * @param time			Time to hold the menu for.
 * @return				True on success, false on failure.
 * @error				Invalid Handle.
 */
native bool:SendPanelToClient(Handle:panel, client, MenuHandler:handler, time);

/**
 * @brief Returns the amount of text the menu can still hold.  If this is 
 * limit is reached or overflowed, the text is silently truncated.  
 *
 * Radio menus: Currently 511 characters (512 bytes).
 * Valve menus: Currently -1 (no meaning).
 *
 * @param panel			A MenuPanel Handle.
 * @return				Number of characters that the menu can still hold, 
 *						or -1 if there is no known limit.
 * @error				Invalid Handle.
 */
native GetPanelTextRemaining(Handle:panel);

/**
 * @brief Returns the current key position.
 *
 * @param panel			A MenuPanel Handle.
 * @return				Current key position starting at 1.
 * @error				Invalid Handle.
 */
native GetPanelCurrentKey(Handle:panel);

/**
 * @brief Sets the next key position.  This cannot be used
 * to traverse backwards.
 *
 * @param panel			A MenuPanel Handle.
 * @param key			Key that is greater or equal to
 *						GetPanelCurrentKey().
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:SetPanelCurrentKey(Handle:panel, key);

/**
 * @brief Redraws menu text from inside a MenuAction_DisplayItem callback.
 * 
 * @param text			Menu text to draw.
 * @return				Item position; must be returned via the callback.
 */
native RedrawMenuItem(const String:text[]);

/**
 * This function is provided for legacy code only.  Some older plugins may use 
 * network messages instead of the panel API.  This function wraps the panel 
 * API for eased portability into the SourceMod menu system.  
 *
 * This function is only usable with the Radio Menu style.  You do not need to 
 * split up your menu into multiple packets; SourceMod will break the string 
 * up internally.
 *
 * @param client		Client index.
 * @param str			Full menu string as would be passed over the network. 
 * @param time			Time to hold the menu for.
 * @param keys			Selectable key bitstring.
 * @param handler		Optional handler function, with the same rules as
 *						SendPanelToClient().
 * @return				True on success, false on failure.
 * @error				Invalid client index, or radio menus not supported.
 */
native bool:InternalShowMenu(client, const String:str[], time, keys=-1, MenuHandler:handler=MenuHandler:-1);

/**
 * Retrieves voting information from MenuAction_VoteEnd.
 *
 * @param param2		Second parameter of MenuAction_VoteEnd.
 * @param winningVotes	Number of votes received by the winning option.
 * @param totalVotes	Number of total votes received.
 * @noreturn
 */
stock GetMenuVoteInfo(param2, &winningVotes, &totalVotes)
{
	winningVotes = param2 & 0xFFFF;
	totalVotes = param2 >> 16;
}

/**
 * Quick stock to determine whether voting is allowed.  This doesn't let you 
 * fine-tune a reason for not voting, so it's not recommended for lazily 
 * telling clients that voting isn't allowed.
 * 
 * @return				True if voting is allowed, false if voting is in progress
 *						or the cooldown is active.
 */
stock bool:IsNewVoteAllowed()
{
	if (IsVoteInProgress() || CheckVoteDelay() != 0)
	{
		return false;
	}
	
	return true;
}

