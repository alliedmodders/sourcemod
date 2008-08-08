/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Sample Extension
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

#include "StructHandle.h"
#include "extension.h"

bool StructHandle::GetInt( const char *member, int *value )
{
	int offset;
	
	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Int8:
		{
			*value = *(uint8 *)((unsigned char *)data + offset);
			break;
		}

		case Member_Int16:
		{
			*value = *(uint16 *)((unsigned char *)data + offset);
			break;
		}

		case Member_Int32:
		{
			*value = *(uint32 *)((unsigned char *)data + offset);
			break;
		}

		case Member_Int8Pointer:
		{
			*value = **(uint8 **)((unsigned char *)data + offset);
			break;
		}

		case Member_Int16Pointer:
		{
			*value = **(uint16 **)((unsigned char *)data + offset);
			break;
		}

		case Member_Int32Pointer:
		{
			*value = **(uint32 **)((unsigned char *)data + offset);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::GetFloat( const char *member, float *value )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Float:
		{
			*value = *(float *)((unsigned char *)data + offset);
			break;
		}

		case Member_FloatPointer:
		{
			*value = **(float **)((unsigned char *)data + offset);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetInt( const char *member, int value )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Int8:
		{
			*(uint8 *)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_Int16:
		{
			*(uint16 *)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_Int32:
		{
			*(uint32 *)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_Int8Pointer:
		{
			**(uint8 **)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_Int16Pointer:
		{
			**(uint16 **)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_Int32Pointer:
		{
			**(uint32 **)((unsigned char *)data + offset) = value;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetFloat( const char *member, float value )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Float:
		{
			*(float *)((unsigned char *)data + offset) = value;
			break;
		}

		case Member_FloatPointer:
		{
			**(float **)((unsigned char *)data + offset) = value;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::IsPointerNull( const char *member, bool *result )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Int8Pointer:
		case Member_Int16Pointer:
		case Member_Int32Pointer:
		case Member_FloatPointer:
		case Member_CharPointer:
		case Member_VectorPointer:
		case Member_EHandlePointer:
		{
			*result = *((unsigned char **)data + offset) == NULL;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::setPointerNull( const char *member )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Int8Pointer:
		case Member_Int16Pointer:
		case Member_Int32Pointer:
		case Member_FloatPointer:
		case Member_CharPointer:
		case Member_VectorPointer:
		case Member_EHandlePointer:
		{
			*((unsigned char **)data + offset) = NULL;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::GetVector( const char *member, Vector *vec )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Vector:
		{
			*vec = *(Vector *)((unsigned char *)data + offset);
			break;
		}

		case Member_VectorPointer:
		{
			*vec = **(Vector **)((unsigned char *)data + offset);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetVector( const char *member, Vector vec )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Vector:
		{
			*(Vector *)((unsigned char *)data + offset) = vec;
			break;
		}

		case Member_VectorPointer:
		{
			**(Vector **)((unsigned char *)data + offset) = vec;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::GetString( const char *member, char **string )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Char:
		{
			*string = (char *)((unsigned char *)data + offset);
			break;
		}

		case Member_CharPointer:
		{
			*string = *(char **)((unsigned char *)data + offset);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetString( const char *member, char *string )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Char:
		{
			int size;
			info->getSize(member, &size);
			char *location = (char *)data + offset;
			UTIL_Format(location, size, "%s", string);
			break;
		}

		case Member_CharPointer:
		{
			int size;
			info->getSize(member, &size);
			char *location = *(char **)((unsigned char *)data + offset);
			UTIL_Format(location, size, "%s", string);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::GetEHandle( const char *member, CBaseHandle **handle )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_EHandle:
		{
			*handle = (CBaseHandle *)((unsigned char *)data + offset);
			break;
		}

		case Member_EHandlePointer:
		{
			*handle = *(CBaseHandle **)((unsigned char *)data + offset);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetEHandle( const char *member, CBaseHandle *handle )
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type))
	{
		return false;
	}

	switch (type)
	{
		case Member_Float:
		{
			*(CBaseHandle *)((unsigned char *)data + offset) = *handle;
			break;
		}

		case Member_FloatPointer:
		{
			**(CBaseHandle **)((unsigned char *)data + offset) = *handle;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

