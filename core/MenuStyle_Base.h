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

#ifndef _INCLUDE_MENUSTYLE_BASE_H
#define _INCLUDE_MENUSTYLE_BASE_H

#include <IMenuManager.h>
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_memtable.h"

using namespace SourceMod;
using namespace SourceHook;

class CItem
{
public:
	CItem()
	{
		infoString = -1;
		displayString = -1;
		style = 0;
	}
public:
	int infoString;
	int displayString;
	unsigned int style;
};

class CBaseMenu : public IBaseMenu
{
public:
	CBaseMenu(IMenuStyle *pStyle);
	virtual ~CBaseMenu();
public:
	virtual bool AppendItem(const char *info, const ItemDrawInfo &draw);
	virtual bool InsertItem(unsigned int position, const char *info, const ItemDrawInfo &draw);
	virtual bool RemoveItem(unsigned int position);
	virtual void RemoveAllItems();
	virtual const char *GetItemInfo(unsigned int position, ItemDrawInfo *draw=NULL);
	virtual unsigned int GetItemCount();
	virtual bool SetPagination(unsigned int itemsPerPage);
	virtual unsigned int GetPagination();
	virtual IMenuStyle *GetDrawStyle();
	virtual void SetDefaultTitle(const char *message);
	virtual const char *GetDefaultTitle();
	virtual bool GetExitButton();
	virtual bool SetExitButton(bool set);
protected:
	String m_Title;
	IMenuStyle *m_pStyle;
	unsigned int m_Pagination;
	CVector<CItem> m_items;
	BaseStringTable m_Strings;
	bool m_ExitButton;
};

#endif //_INCLUDE_MENUSTYLE_BASE_H
