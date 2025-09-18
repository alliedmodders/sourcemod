#pragma once

namespace sdk {

class CBaseEntity;
class edict_t;

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

// Demanded by KHook
void Vector_Assign(Vector* assignee, Vector* value) {
	new (assignee) Vector(*value);
}

void Vector_Delete(Vector* assignee) {
	assignee->~Vector();
}

void StringT_Assign(string_t* assignee, string_t* value) {
	new (assignee) string_t(*value);
}

void StringT_Delete(string_t* assignee) {
	assignee->~string_t();
}

}