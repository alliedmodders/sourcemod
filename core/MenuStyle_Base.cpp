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

#include <stdarg.h>
#include "sm_stringutil.h"
#include "MenuStyle_Base.h"

CBaseMenu::CBaseMenu(IMenuStyle *pStyle) : 
m_pStyle(pStyle), m_Strings(512), m_Pagination(7), m_ExitButton(true)
{
}

CBaseMenu::~CBaseMenu()
{
}

bool CBaseMenu::AppendItem(const char *info, const ItemDrawInfo &draw)
{
	if (m_Pagination == MENU_NO_PAGINATION
		&& m_items.size() >= m_pStyle->GetMaxPageItems())
	{
		return false;
	}

	CItem item;

	item.infoString = m_Strings.AddString(info);
	if (draw.display)
	{
		item.displayString = m_Strings.AddString(draw.display);
	}
	item.style = draw.style;


	m_items.push_back(item);

	return true;
}

bool CBaseMenu::InsertItem(unsigned int position, const char *info, const ItemDrawInfo &draw)
{
	if (m_Pagination == MENU_NO_PAGINATION
		&& m_items.size() >= m_pStyle->GetMaxPageItems())
	{
		return false;
	}

	if (position >= m_items.size())
	{
		return false;
	}

	CItem item;
	item.infoString = m_Strings.AddString(info);
	if (draw.display)
	{
		item.displayString = m_Strings.AddString(draw.display);
	}
	item.style = draw.style;

	CVector<CItem>::iterator iter = m_items.iterAt(position);
	m_items.insert(iter, item);

	return true;
}

bool CBaseMenu::RemoveItem(unsigned int position)
{
	if (position >= m_items.size())
	{
		return false;
	}

	m_items.erase(m_items.iterAt(position));

	if (m_items.size() == 0)
	{
		m_Strings.Reset();
	}

	return true;
}

void CBaseMenu::RemoveAllItems()
{
	m_items.clear();
	m_Strings.Reset();
}

const char *CBaseMenu::GetItemInfo(unsigned int position, ItemDrawInfo *draw/* =NULL */)
{
	if (position >= m_items.size())
	{
		return NULL;
	}

	if (draw)
	{
		draw->display = m_Strings.GetString(m_items[position].displayString);
		draw->style = m_items[position].style;
	}

	return m_Strings.GetString(m_items[position].infoString);
}

unsigned int CBaseMenu::GetItemCount()
{
	return m_items.size();
}

bool CBaseMenu::SetPagination(unsigned int itemsPerPage)
{
	if (itemsPerPage > 7)
	{
		return false;
	}

	m_Pagination = itemsPerPage;

	return true;
}

unsigned int CBaseMenu::GetPagination()
{
	return m_Pagination;
}

IMenuStyle *CBaseMenu::GetDrawStyle()
{
	return m_pStyle;
}

void CBaseMenu::SetDefaultTitle(const char *message)
{
	m_Title.assign(message);
}

const char *CBaseMenu::GetDefaultTitle()
{
	return m_Title.c_str();
}

bool CBaseMenu::GetExitButton()
{
	return m_ExitButton;
}

bool CBaseMenu::SetExitButton(bool set)
{
	m_ExitButton = set;
	return true;
}
