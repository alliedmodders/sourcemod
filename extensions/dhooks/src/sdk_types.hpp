#pragma once

#include <cstdint>

namespace sdk {

// Verified IGameSystem::Shutdown to be the same on :
// - Counter-Strike : Source
// - Day of Defeat
// - Half-Life 2 Deathmatch
// - Team Fortess 2
// - Left 4 Dead
// - Left 4 Dead 2
// - Insurgency
// - Black Mesa
// - Day of Infamy
// - Dystopia
// - Empires
// - GMod
// - Half-Life Deathmatch : Source
// - IOSoccer
// - JB Mod
// - Nuclear Dawn
// - No More Room In Hell
// - Team Fortress 2 Classic
// - Zombie Panic! : Source
class IGameSystem
{
public:
	virtual char const *Name() = 0;
	virtual bool Init() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;
};
// Except The Ship. Because of course it had to be special
class IGameSystem_TheShip
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;
};

class CBaseEntity;

static constexpr std::size_t FL_EDICT_FREE = (1<<1);
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