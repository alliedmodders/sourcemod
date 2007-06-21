/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
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
	MenuAction_End = (1<<4),		/**< A menu display has fully ended (param1=reason) */
	MenuAction_VoteEnd = (1<<5),	/**< (VOTE ONLY): A vote sequence has succeeded (param1=chosen item) */
	MenuAction_VoteStart = (1<<6), 	/**< (VOTE ONLY): A vote sequence has started (nothing passed) */
	MenuAction_VoteCancel = (1<<7),	/**< (VOTE ONLY): A vote sequence has been cancelled (nothing passed) */
};

/** Default menu actions */
#define MENU_ACTIONS_DEFAULT	MenuAction_Select|MenuAction_Cancel|MenuAction_End
/** All menu actions */
#define MENU_ACTIONS_ALL		-1

#define MENU_NO_PAGINATION		0		/**< Menu should not be paginated (10 items max) */
#define MENU_TIME_FOREVER		0		/**< Menu should be displayed as long as possible */

#define ITEMDRAW_DEFAULT		(0)		/**< Item should be drawn normally */
#define ITEMDRAW_DISABLED		(1<<0)	/**< Item is drawn but not selectable */
#define ITEMDRAW_RAWLINE		(1<<1)	/**< Item should be a raw line, without a slot */
#define ITEMDRAW_NOTEXT			(1<<2)	/**< No text should be drawn */
#define ITEMDRAW_SPACER			(1<<3)	/**< Item should be drawn as a spacer, if possible */
#define ITEMDRAW_IGNORE	((1<<1)|(1<<2))	/**< Item should be completely ignored (rawline + notext) */
#define ITEMDRAW_CONTROL		(1<<4)	/**< Item is control text (back/next/exit) */

/**
 * Reasons a menu can be cancelled.
 */
enum
{
	MenuCancel_Disconnect = -1,			/**< Client dropped from the server */
	MenuCancel_Interrupt = -2,			/**< Client was interrupted with another menu */
	MenuCancel_Exit = -3,				/**< Client selected "exit" on a paginated menu */
	MenuCancel_NoDisplay = -4,			/**< Menu could not be displayed to the client */
};

/**
 * Reasons a menu ended.
 */
enum
{
	MenuEnd_Cancelled = -1,				/**< Menu was cancelled, reason was passed in MenuAction_Cancel */
	MenuEnd_Exit = -2,					/**< Menu was cleanly exited (but cancelled) */
	MenuEnd_Selected = -3,				/**< Menu item was selected and thus the menu is finished */
	MenuEnd_VotingDone = -4,			/**< Voting finished */
	MenuEnd_VotingCancelled = -5,		/**< Voting was cancelled */
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
functag MenuHandler public(Handle:menu, MenuAction:action, param1, param2);

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
 * Appends a new item to the end of a menu.
 *
 * @param menu				Menu Handle.
 * @param info				Item information string.
 * @param display			Default item display string.
 * @param style				Drawing style flags.
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
 * @param style				Drawing style flags.
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
native bool:GetMenuItem(Handle:menu, position, String:infoBuf[], infoBufLen, &style=0, String:dispBuf[]="", dispBufLen=0);

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
 * @param menu				Handle to the menu.
 * @param itemsPerPage		Number of items per page, or MENU_NO_PAGINATION.
 * @return					True on success, false if pagination is too high or low.
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
 * Sets whether or not the menu has an exit button.
 * By default, menus have an exit button.
 *
 * @param menu				Menu Handle.
 * @param button			True to enable the button, false to remove it.
 * @return 					True if allowed; false on failure.
 * @error					Invalid Handle.
 */
native bool:SetMenuExitButton(Handle:menu, bool:button);

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
 * Returns whether a vote is in progress on the given menu.
 *
 * @param menu				Menu Handle.
 * @return					True if a vote is in progress, false otherwise.
 */
native bool:IsVoteInProgress(Handle:menu);

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
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle.
 */
native bool:VoteMenu(Handle:menu, clients[], numClients, time);

/**
 * Sends a vote menu to all clients.  See VoteMenu() for more information.
 *
 * @param menu				Menu Handle.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle.
 */
stock VoteMenuToAll(Handle:menu, time)
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
	
	return VoteMenu(menu, players, total, time);
}

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
 * Returns whether a client is viewing a menu.  If the menu source 
 * is MenuSource_Normal, a menu Handle will also be returned.
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
 * @param panel			A MenuPanel Handle.
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
