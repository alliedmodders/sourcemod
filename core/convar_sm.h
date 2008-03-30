//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-08-13 06:34:30 -0500 (Sun, 13 Aug 2006) $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//=============================================================================//

#ifndef CONVAR_H
#define CONVAR_H
#if _WIN32
#pragma once
#endif

#include "tier0/dbg.h"

#ifdef _WIN32
#define FORCEINLINE_CVAR FORCEINLINE
#elif _LINUX
#define FORCEINLINE_CVAR __inline__ FORCEINLINE
#else
#error "implement me"
#endif

// The default, no flags at all
#define FCVAR_NONE				0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_UNREGISTERED		(1<<0)	// If this is set, don't add to linked list, etc.
#define FCVAR_LAUNCHER			(1<<1)	// defined by launcher
#define FCVAR_GAMEDLL			(1<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1<<3)  // defined by the client DLL
#define FCVAR_MATERIAL_SYSTEM	(1<<4)	// Defined by the material system.
#define FCVAR_DATACACHE			(1<<19)	// Defined by the datacache system.
#define FCVAR_STUDIORENDER		(1<<15)	// Defined by the studiorender system.
#define FCVAR_FILESYSTEM		(1<<21)	// Defined by the file system.
#define FCVAR_PLUGIN			(1<<18)	// Defined by a 3rd party plugin.
#define FCVAR_TOOLSYSTEM		(1<<20)	// Defined by an IToolSystem library
#define FCVAR_SOUNDSYSTEM		(1<<23)	// Defined by the soundsystem library
#define FCVAR_INPUTSYSTEM		(1<<25)	// Defined by the inputsystem dll
#define FCVAR_NETWORKSYSTEM		(1<<26) // Defined by the network system
// NOTE!! if you add a cvar system, add it here too!!!!
// the engine lacks a cvar flag, but needs it for xbox
// an engine cvar is thus a cvar not marked with any other system
#define FCVAR_NON_ENGINE		((FCVAR_LAUNCHER|FCVAR_GAMEDLL|FCVAR_CLIENTDLL|FCVAR_MATERIAL_SYSTEM|FCVAR_DATACACHE|FCVAR_STUDIORENDER|FCVAR_FILESYSTEM|FCVAR_PLUGIN|FCVAR_TOOLSYSTEM|FCVAR_SOUNDSYSTEM|FCVAR_INPUTSYSTEM|FCVAR_NETWORKSYSTEM))

// ConVar only
#define FCVAR_PROTECTED			(1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY			(1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1<<9)	// changes the client's info string
#define FCVAR_CHEAT				(1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats

#define FCVAR_PRINTABLEONLY		(1<<10)  // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED			(1<<11)  // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_NEVER_AS_STRING	(1<<12)  // never try to print that cvar

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED		(1<<13)	// server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_DEMO				(1<<16)  // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1<<17)  // don't record these command in demofiles

#define FCVAR_NOT_CONNECTED		(1<<22)	// cvar cannot be changed by a client that is connected to a server

#define FCVAR_ARCHIVE_XBOX		(1<<24) // cvar written to config.cfg on the Xbox


// #define FCVAR_AVAILABLE			(1<<27)
// #define FCVAR_AVAILABLE			(1<<28)
// #define FCVAR_AVAILABLE			(1<<29)
// #define FCVAR_AVAILABLE			(1<<30)
// #define FCVAR_AVAILABLE			(1<<31)


class ConVar;
class ConCommand;
class ConCommandBase;

// Any executable that wants to use ConVars need to implement one of
// these to hook up access to console variables.
class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase( ConCommandBase *pVar )=0;
};


// You don't have to instantiate one of these, just call its 
// OneTimeInit function when your executable is initializing.
class ConCommandBaseMgr
{
public:
	// Call this ONCE when the executable starts up.
	static void	OneTimeInit( IConCommandBaseAccessor *pAccessor );
#ifdef _XBOX
	static bool Fixup( ConCommandBase* pConCommandBase );
#ifndef _RETAIL
	static void PublishCommands( bool bForce );
#endif
#endif
};

// Called when a ConVar changes value
typedef void ( *FnChangeCallback )( ConVar *var, char const *pOldString );

// Called when a ConCommand needs to execute
typedef void ( *FnCommandCallback )( void );

#define COMMAND_COMPLETION_MAXITEMS		64
#define COMMAND_COMPLETION_ITEM_LENGTH	64

// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
typedef int  ( *FnCommandCompletionCallback )( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
	friend class ConCommandBaseMgr;
	friend class CCvar;
	friend class ConVar;
	friend class ConCommand;

public:
								ConCommandBase( void );
								ConCommandBase( char const *pName, char const *pHelpString = 0, 
									int flags = 0 );

	virtual						~ConCommandBase( void );

	virtual	bool				IsCommand( void ) const;

	// Check flag
	virtual bool				IsBitSet( int flag ) const;
	// Set flag
	virtual void				AddFlags( int flags );

	// Return name of cvar
	virtual char const			*GetName( void ) const;

	// Return help text for cvar
	virtual char const			*GetHelpText( void ) const;

	// Deal with next pointer
	const ConCommandBase		*GetNext( void ) const;
	void						SetNext( ConCommandBase *next );
	
	virtual bool				IsRegistered( void ) const;

	// Global methods
	static ConCommandBase const	*GetCommands( void );
	static void					AddToList( ConCommandBase *var );
	static void					RemoveFlaggedCommands( int flag );
	static void					RevertFlaggedCvars( int flag );
	static ConCommandBase const	*FindCommand( char const *name );

protected:
	virtual void				Create( char const *pName, char const *pHelpString = 0, 
									int flags = 0 );

	// Used internally by OneTimeInit to initialize.
	virtual void				Init();

	// Internal copy routine ( uses new operator from correct module )
	char						*CopyString( char const *from );

	// Next ConVar in chain
	ConCommandBase				*m_pNext;

private:
	// Has the cvar been added to the global list?
	bool						m_bRegistered;

	// Static data
	char const 					*m_pszName;
	char const 					*m_pszHelpString;
	
	// ConVar flags
	int							m_nFlags;

protected:

	// ConVars add themselves to this list for the executable. Then ConVarMgr::Init() runs through 
	// all the console variables and registers them.
	static ConCommandBase		*s_pConCommandBases;

	// ConVars in this executable use this 'global' to access values.
	static IConCommandBaseAccessor	*s_pAccessor;
	
public: // Hackalicous
	inline int GetFlags() const
	{
		return m_nFlags;
	}
	inline void SetFlags(int flags)	
	{ 
		m_nFlags = flags; 
	}
};

//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
friend class ConCommandBaseMgr;
friend class CCvar;
#ifdef _STATIC_LINKED
friend class G_ConCommand;
friend class C_ConCommand;
friend class M_ConCommand;
friend class S_ConCommand;
friend class D_ConCommand;
#endif

public:
	typedef ConCommandBase BaseClass;

								ConCommand( void );
								ConCommand( char const *pName, FnCommandCallback callback, 
									char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );

	virtual						~ConCommand( void );

	virtual	bool				IsCommand( void ) const;

	virtual int					AutoCompleteSuggest( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );

	virtual bool				CanAutoComplete( void );

	// Invoke the function
	virtual void				Dispatch( void );
private:
	virtual void				Create( char const *pName, FnCommandCallback callback, 
									char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );

	// Call this function when executing the command
	FnCommandCallback			m_fnCommandCallback;

	FnCommandCompletionCallback	m_fnCompletionCallback;
	bool						m_bHasCompletionCallback;
public: // Hackalicous
	inline FnCommandCallback GetCallback() const
	{ 
		return m_fnCommandCallback; 
	}
};

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase
{
friend class ConCommandBaseMgr;
friend class CCvar;
friend class CDefaultCvar;
#ifdef _STATIC_LINKED
friend class G_ConVar;
friend class C_ConVar;
friend class M_ConVar;
friend class S_ConVar;
friend class D_ConVar;
#endif

public:
	typedef ConCommandBase BaseClass;

								ConVar( char const *pName, char const *pDefaultValue, int flags = 0);

								ConVar( char const *pName, char const *pDefaultValue, int flags, 
									char const *pHelpString );
								ConVar( char const *pName, char const *pDefaultValue, int flags, 
									char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax );
								ConVar( char const *pName, char const *pDefaultValue, int flags, 
									char const *pHelpString, FnChangeCallback callback );
								ConVar( char const *pName, char const *pDefaultValue, int flags, 
									char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
									FnChangeCallback callback );

	virtual						~ConVar( void );

	virtual bool				IsBitSet( int flag ) const;
	virtual char const*			GetHelpText( void ) const;
	virtual bool				IsRegistered( void ) const;
	virtual char const			*GetName( void ) const;
	virtual void				AddFlags( int flags );
	virtual	bool				IsCommand( void ) const;

	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback( FnChangeCallback callback );

	// Retrieve value
	FORCEINLINE_CVAR float			GetFloat( void ) const;
	FORCEINLINE_CVAR int				GetInt( void ) const;
	FORCEINLINE_CVAR bool			GetBool() const {  return !!GetInt(); }
	FORCEINLINE_CVAR char const	   *GetString( void ) const;

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.
	
	// These just call into the IConCommandBaseAccessor to check flags and set the var (which ends up calling InternalSetValue).
	virtual void				SetValue( char const *value );
	virtual void				SetValue( float value );
	virtual void				SetValue( int value );
	
	// Reset to default value
	void						Revert( void );

	// True if it has a min/max setting
	bool						GetMin( float& minVal ) const;
	bool						GetMax( float& maxVal ) const;
	char const					*GetDefault( void ) const;

	static void					RevertAll( void );
private:
	// Called by CCvar when the value of a var is changing.
	virtual void				InternalSetValue(char const *value);
	// For CVARs marked FCVAR_NEVER_AS_STRING
	virtual void				InternalSetFloatValue( float fNewValue );
	virtual void				InternalSetIntValue( int nValue );

	virtual bool				ClampValue( float& value );
	virtual void				ChangeStringValue( char const *tempVal );

	virtual void				Create( char const *pName, char const *pDefaultValue, int flags = 0,
									char const *pHelpString = 0, bool bMin = false, float fMin = 0.0,
									bool bMax = false, float fMax = false, FnChangeCallback callback = 0 );

	// Used internally by OneTimeInit to initialize.
	virtual void				Init();

private:

	// This either points to "this" or it points to the original declaration of a ConVar.
	// This allows ConVars to exist in separate modules, and they all use the first one to be declared.
	// m_pParent->m_pParent must equal m_pParent (ie: m_pParent must be the root, or original, ConVar).
	ConVar						*m_pParent;

	// Static data
	char const					*m_pszDefaultValue;
	
	// Value
	// Dynamically allocated
	char						*m_pszString;
	int							m_StringLength;

	// Values
	float						m_fValue;
	int							m_nValue;

	// Min/Max values
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;
	
	// Call this function when ConVar changes
	FnChangeCallback			m_fnChangeCallback;
public: // Hackalicous
	inline FnChangeCallback	GetCallback() const
	{
		return m_fnChangeCallback;
	}
	inline void SetMin(bool set, float min=0.0)
	{ 
		m_bHasMin = set; 
		m_fMinVal = min; 
	}
	inline void SetMax(bool set, float max=0.0)
	{
		m_bHasMax = set;
		m_fMaxVal = max; 
	}
};


//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVar::GetFloat( void ) const
{
	return m_pParent->m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVar::GetInt( void ) const 
{
	return m_pParent->m_nValue;
}


//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
// Output : char const *
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR char const *ConVar::GetString( void ) const 
{
	if ( m_nFlags & FCVAR_NEVER_AS_STRING )
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	return ( m_pParent->m_pszString ) ? m_pParent->m_pszString : "";
}


#ifdef _STATIC_LINKED
// identifies subsystem via piggybacking constructors with flags
class G_ConCommand : public ConCommand
{
public:
	G_ConCommand(char const *pName, FnCommandCallback callback, char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 ) :	ConCommand(pName, callback, pHelpString, flags|FCVAR_GAMEDLL, completionFunc) {}
};

class C_ConCommand : public ConCommand
{
public:
	C_ConCommand(char const *pName, FnCommandCallback callback, char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 ) :	ConCommand(pName, callback, pHelpString, flags|FCVAR_CLIENTDLL, completionFunc) {}
};

class M_ConCommand : public ConCommand
{
public:
	M_ConCommand(char const *pName, FnCommandCallback callback, char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 ) :	ConCommand(pName, callback, pHelpString, flags|FCVAR_MATERIAL_SYSTEM, completionFunc) {}
};

class S_ConCommand : public ConCommand
{
public:
	S_ConCommand(char const *pName, FnCommandCallback callback, char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 ) :	ConCommand(pName, callback, pHelpString, flags|FCVAR_STUDIORENDER, completionFunc) {}
};

class D_ConCommand : public ConCommand
{
public:
	D_ConCommand(char const *pName, FnCommandCallback callback, char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 ) :	ConCommand(pName, callback, pHelpString, flags|FCVAR_DATACACHE, completionFunc) {}
};

typedef void ( *G_FnChangeCallback )( G_ConVar *var, char const *pOldString );
typedef void ( *C_FnChangeCallback )( C_ConVar *var, char const *pOldString );
typedef void ( *M_FnChangeCallback )( M_ConVar *var, char const *pOldString );
typedef void ( *S_FnChangeCallback )( S_ConVar *var, char const *pOldString );
typedef void ( *D_FnChangeCallback )( D_ConVar *var, char const *pOldString );

class G_ConVar : public ConVar
{
public:
	G_ConVar( char const *pName, char const *pDefaultValue, int flags = 0) : ConVar(pName, pDefaultValue, flags|FCVAR_GAMEDLL) {}
	G_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString ) : ConVar(pName, pDefaultValue, flags|FCVAR_GAMEDLL, pHelpString ) {}
	G_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) : ConVar(pName, pDefaultValue, flags|FCVAR_GAMEDLL, pHelpString, bMin, fMin, bMax, fMax) {}
	G_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, G_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_GAMEDLL, pHelpString, (FnChangeCallback)callback ) {}
	G_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax, G_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_GAMEDLL, pHelpString, bMin, fMin, bMax, fMax, (FnChangeCallback)callback ) {}
};

class C_ConVar : public ConVar
{
public:
	C_ConVar( char const *pName, char const *pDefaultValue, int flags = 0) : ConVar(pName, pDefaultValue, flags|FCVAR_CLIENTDLL) {}
	C_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString ) : ConVar(pName, pDefaultValue, flags|FCVAR_CLIENTDLL, pHelpString ) {}
	C_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) : ConVar(pName, pDefaultValue, flags|FCVAR_CLIENTDLL, pHelpString, bMin, fMin, bMax, fMax) {}
	C_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, C_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_CLIENTDLL, pHelpString, (FnChangeCallback)callback ) {}
	C_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax, C_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_CLIENTDLL, pHelpString, bMin, fMin, bMax, fMax, (FnChangeCallback)callback ) {}
};

class M_ConVar : public ConVar
{
public:
	M_ConVar( char const *pName, char const *pDefaultValue, int flags = 0) : ConVar(pName, pDefaultValue, flags|FCVAR_MATERIAL_SYSTEM) {}
	M_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString ) : ConVar(pName, pDefaultValue, flags|FCVAR_MATERIAL_SYSTEM, pHelpString ) {}
	M_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) : ConVar(pName, pDefaultValue, flags|FCVAR_MATERIAL_SYSTEM, pHelpString, bMin, fMin, bMax, fMax) {}
	M_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, M_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_MATERIAL_SYSTEM, pHelpString, (FnChangeCallback)callback ) {}
	M_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax, M_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_MATERIAL_SYSTEM, pHelpString, bMin, fMin, bMax, fMax, (FnChangeCallback)callback ) {}
};

class S_ConVar : public ConVar
{
public:
	S_ConVar( char const *pName, char const *pDefaultValue, int flags = 0) : ConVar(pName, pDefaultValue, flags|FCVAR_STUDIORENDER) {}
	S_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString ) : ConVar(pName, pDefaultValue, flags|FCVAR_STUDIORENDER, pHelpString ) {}
	S_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) : ConVar(pName, pDefaultValue, flags|FCVAR_STUDIORENDER, pHelpString, bMin, fMin, bMax, fMax) {}
	S_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, M_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_STUDIORENDER, pHelpString, (FnChangeCallback)callback ) {}
	S_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax, S_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_STUDIORENDER, pHelpString, bMin, fMin, bMax, fMax, (FnChangeCallback)callback ) {}
};

class D_ConVar : public ConVar
{
public:
	D_ConVar( char const *pName, char const *pDefaultValue, int flags = 0) : ConVar(pName, pDefaultValue, flags|FCVAR_DATACACHE) {}
	D_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString ) : ConVar(pName, pDefaultValue, flags|FCVAR_DATACACHE, pHelpString ) {}
	D_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) : ConVar(pName, pDefaultValue, flags|FCVAR_DATACACHE, pHelpString, bMin, fMin, bMax, fMax) {}
	D_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, M_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_DATACACHE, pHelpString, (FnChangeCallback)callback ) {}
	D_ConVar( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax, D_FnChangeCallback callback ) : ConVar(pName, pDefaultValue, flags|FCVAR_DATACACHE, pHelpString, bMin, fMin, bMax, fMax, (FnChangeCallback)callback ) {}
};

// redirect these declarations to their specific subsystem
#ifdef GAME_DLL
#define ConCommand	G_ConCommand
#define ConVar		G_ConVar
#endif
#ifdef CLIENT_DLL
#define ConCommand	C_ConCommand
#define ConVar		C_ConVar
#endif
#ifdef MATERIALSYSTEM_DLL
#define ConCommand	M_ConCommand
#define ConVar		M_ConVar
#endif
#ifdef STUDIORENDER_DLL
#define ConCommand	S_ConCommand
#define ConVar		S_ConVar
#endif
#ifdef DATACACHE_DLL
#define ConCommand	D_ConCommand
#define ConVar		D_ConVar
#endif
#endif // _STATIC_LINKED

//-----------------------------------------------------------------------------
// Purpose: Utility to quicky generate a simple console command
//-----------------------------------------------------------------------------
#define CON_COMMAND( name, description ) \
   static void name(); \
   static ConCommand name##_command( #name, name, description ); \
   static void name()

//-----------------------------------------------------------------------------
// Purpose: Utility to quicky generate a simple console command
//-----------------------------------------------------------------------------
#define CON_COMMAND_F( name, description, flags ) \
   static void name(); \
   static ConCommand name##_command( #name, name, description, flags ); \
   static void name()


#endif // CONVAR_H
