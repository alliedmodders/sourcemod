/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SDK_HACKS_H_
#define _INCLUDE_SDK_HACKS_H_

class SDKVector
{
public:
	SDKVector(float x1, float y1, float z1)
	{
		this->x = x1;
		this->y = y1;
		this->z = z1;
	}
	SDKVector(void)
	{
		this->x = 0.0;
		this->y = 0.0;
		this->z = 0.0;
	}
	float x;
	float y;
	float z;
};

struct string_t
{
public:
	bool operator!() const							{ return ( pszValue == NULL );			}
	bool operator==( const string_t &rhs ) const	{ return ( pszValue == rhs.pszValue );	}
	bool operator!=( const string_t &rhs ) const	{ return ( pszValue != rhs.pszValue );	}
	bool operator<( const string_t &rhs ) const		{ return ((void *)pszValue < (void *)rhs.pszValue ); }

	const char *ToCStr() const						{ return ( pszValue ) ? pszValue : ""; 	}
 	
protected:
	const char *pszValue;
};

struct castable_string_t : public string_t // string_t is used in unions, hence, no constructor allowed
{
	castable_string_t()							{ pszValue = NULL; }
	castable_string_t( const char *pszFrom )	{ pszValue = (pszFrom && *pszFrom) ? pszFrom : 0; }
};

#define NULL_STRING			castable_string_t()
#define STRING( string_t_obj )	(string_t_obj).ToCStr()
#define MAKE_STRING( c_str )	castable_string_t( c_str )

#define FL_EDICT_FREE		(1<<1)

struct edict_t
{
public:
	bool IsFree()
	{
		return (m_fStateFlags & FL_EDICT_FREE) != 0;
	}
private:
	int	m_fStateFlags;	
};

class CBaseHandle
{
/*public:
	bool IsValid() const {return m_Index != INVALID_EHANDLE_INDEX;}
	int GetEntryIndex() const
	{
		if ( !IsValid() )
			return NUM_ENT_ENTRIES-1;
		return m_Index & ENT_ENTRY_MASK;
	}*/
private:
	unsigned long	m_Index;
};

#endif