
#ifndef _INCLUDE_SOURCEMOD_DETOURS_H_
#define _INCLUDE_SOURCEMOD_DETOURS_H_

#include "extension.h"
#include <jit/jit_helpers.h>
#include <jit/x86/x86_macros.h>
#include "detourhelpers.h"

/**
 * CDetours class for SourceMod Extensions by pRED*
 * detourhelpers.h entirely stolen from CSS:DM and were written by BAILOPAN (I assume).
 * asm.h/c from devmaster.net (thanks cybermind)
 */

class CDetourManager;

class CDetour
{
public:

	bool IsEnabled();

	/**
	 * These would be somewhat self-explanatory I hope
	 */
	void EnableDetour();
	void DisableDetour();

	friend class CDetourManager;

protected:
	CDetour(void *callbackfunction, size_t paramsize, const char *signame);
	~CDetour();

	bool Init(ISourcePawnEngine *spengine, IGameConfig *gameconf);
private:

	/* These create/delete the allocated memory */
	bool CreateDetour();
	void DeleteDetour();

	bool enabled;
	bool detoured;

	patch_t detour_restore;
	void *detour_address;
	void *detour_callback;

	const char *signame;

	void *callbackfunction;

	size_t paramsize;

	ISourcePawnEngine *spengine;
	IGameConfig *gameconf;
};

class CBlocker
{
public:
	void EnableBlock(int returnValue = 0);
	void DisableBlock();

	friend class CDetourManager;

protected:
	CBlocker(const char *signame, bool isVoid);
	~CBlocker();

	bool Init(ISourcePawnEngine *spengine, IGameConfig *gameconf);

private:
	bool isValid;
	bool isEnabled;
	bool isVoid;
	patch_t block_restore;
	void *block_address;

	const char *block_sig;

	ISourcePawnEngine *spengine;
	IGameConfig *gameconf;
};

class CDetourManager
{
public:

	/**
	 * Return Types for Detours
	 */
	enum DetourReturn
	{
		DetourReturn_Ignored = 0,		/** Ignore our result and let the original function run */
		DetourReturn_Override = 1,		/** Block the original function from running and use our return value */
	};

	static void Init(ISourcePawnEngine *spengine, IGameConfig *gameconf);

	/**
	 * Creates a new detour
	 * @param callbackfunction			Void pointer to your detour callback function. This should be a static function.
	 *									It should have pointer to the thisptr as the first param and then the same params 
	 *									as the original function. Use void * for unknown types.
	 * @param paramsize					This is usually the number of params the function has (not including thisptr). If the function
	 *									passes complex types by value you need to add the sizeof() the type (aligned to 4 bytes).
	 *									Ie: passing something of size 8 would count as 2 in the param count.
	 * @param signame					Section name containing a signature to fetch from the gamedata file.
	 * @return							A new CDetour pointer to control your detour.
	 *
	 * Example:
	 *
	 * CBaseServer::ConnectClient(netadr_s &, int, int, int, char  const*, char  const*, char  const*, int)
	 *
	 * Callback: 
	 * DetourReturn ConnectClientDetour(void *CBaseServer, void *netaddr_s, int something, int something2, int something3, char  const* name, char  const* pass, const char* steamcert, int len);
	 *
	 * Creation:
	 * CDetourManager::CreateDetour((void *)&ConnectClientDetour, 8, "ConnectClient");
	 */
	static CDetour *CreateDetour(void *callbackfunction, size_t paramsize, const char *signame);

	/**
	 * Deletes a detour
	 */
	static void DeleteDetour(CDetour *detour);

	/**
	 * Creates a function blocker. This is slightly faster than a detour because it avoids a call.
	 *
	 * @param signame					Section name containing a signature to fetch from the gamedata file.
	 * @param isVoid					Specifies if the function can return void.
	 */
	static CBlocker *CreateFunctionBlock(const char *signame, bool isVoid);

	/**
	 * Delete a function blocker.
	 */
	static void DeleteFunctionBlock(CBlocker *block);


	/**
	 * Global DetourReturn value to use for the current hook
	 */
	static int returnValue;

	friend class CBlocker;
	friend class CDetour;

private:
	static ISourcePawnEngine *spengine;
	static IGameConfig *gameconf;
};

typedef bool DetourReturn;

#define DETOUR_RESULT_IGNORED false
#define DETOUR_RESULT_OVERRIDE true

#define SET_DETOUR_RETURN_VALUE(value)		CDetourManager::returnValue=(int)value
#define RETURN_DETOUR(result)				return result
#define RETURN_DETOUR_VALUE(result,value)	do { SET_DETOUR_RETURN_VALUE(value); return (result); } while(0)

#endif // _INCLUDE_SOURCEMOD_DETOURS_H_
