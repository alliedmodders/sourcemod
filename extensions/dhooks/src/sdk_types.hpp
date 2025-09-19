#pragma once

#include <cstdint>

namespace sdk {

class CBaseEntity;

static constexpr size_t FL_EDICT_FREE = (1<<1);
struct edict_t {
public:
	bool IsFree() { return (m_fStateFlags & FL_EDICT_FREE) != 0; }
private:
	int	m_fStateFlags;	
};

class Vector
{
public:
	Vector(float x1, float y1, float z1)
	{
		this->x = x1;
		this->y = y1;
		this->z = z1;
	}
	Vector(void)
	{
		this->x = 0.0;
		this->y = 0.0;
		this->z = 0.0;
	}
	float x;
	float y;
	float z;
};

static constexpr std::uintptr_t NULL_STRING = 0;
struct string_t
{
public:
	bool operator!() const							{ return ( pszValue == nullptr );		}
	bool operator==( const string_t &rhs ) const	{ return ( pszValue == rhs.pszValue );	}
	bool operator!=( const string_t &rhs ) const	{ return ( pszValue != rhs.pszValue );	}
	bool operator<( const string_t &rhs ) const		{ return ((void *)pszValue < (void *)rhs.pszValue ); }

	const char *ToCStr() const						{ return ( pszValue ) ? pszValue : ""; 	}
 	
protected:
	const char *pszValue;
};
static_assert(sizeof(string_t) == sizeof(void*));

}