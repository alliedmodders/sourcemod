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