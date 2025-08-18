#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include <ISDKHooks.h>
#include <convar.h>
#include <list>
#include <am-vector.h>
#include <vtable_hook_helper.h>

#include <iplayerinfo.h>
#include <shareddefs.h>

#if SOURCE_ENGINE >= SE_ORANGEBOX
#include <itoolentity.h>
#endif

#include "takedamageinfohack.h"

#if SOURCE_ENGINE >= SE_CSS && SOURCE_ENGINE != SE_LEFT4DEAD
#define GETMAXHEALTH_IS_VIRTUAL
#endif
#if SOURCE_ENGINE != SE_HL2DM && SOURCE_ENGINE != SE_DODS && SOURCE_ENGINE != SE_CSS && SOURCE_ENGINE != SE_TF2 && \
	SOURCE_ENGINE != SE_LEFT4DEAD2 && SOURCE_ENGINE != SE_CSGO && SOURCE_ENGINE != SE_NUCLEARDAWN && \
	SOURCE_ENGINE != SE_BLADE && SOURCE_ENGINE != SE_MCV
#define GAMEDESC_CAN_CHANGE
#endif


/**
 * Globals
 */

struct HookTypeData
{
	const char *name;
	const char *dtReq;
	bool supported;
	unsigned int offset;
};

enum SDKHookType
{
	SDKHook_EndTouch,
	SDKHook_FireBulletsPost,
	SDKHook_OnTakeDamage,
	SDKHook_OnTakeDamagePost,
	SDKHook_PreThink,
	SDKHook_PostThink,
	SDKHook_SetTransmit,
	SDKHook_Spawn,
	SDKHook_StartTouch,
	SDKHook_Think,
	SDKHook_Touch,
	SDKHook_TraceAttack,
	SDKHook_TraceAttackPost,
	SDKHook_WeaponCanSwitchTo,
	SDKHook_WeaponCanUse,
	SDKHook_WeaponDrop,
	SDKHook_WeaponEquip,
	SDKHook_WeaponSwitch,
	SDKHook_ShouldCollide,
	SDKHook_PreThinkPost,
	SDKHook_PostThinkPost,
	SDKHook_ThinkPost,
	SDKHook_EndTouchPost,
	SDKHook_GroundEntChangedPost,
	SDKHook_SpawnPost,
	SDKHook_StartTouchPost,
	SDKHook_TouchPost,
	SDKHook_VPhysicsUpdate,
	SDKHook_VPhysicsUpdatePost,
	SDKHook_WeaponCanSwitchToPost,
	SDKHook_WeaponCanUsePost,
	SDKHook_WeaponDropPost,
	SDKHook_WeaponEquipPost,
	SDKHook_WeaponSwitchPost,
	SDKHook_Use,
	SDKHook_UsePost,
	SDKHook_Reload,
	SDKHook_ReloadPost,
	SDKHook_GetMaxHealth,
	SDKHook_Blocked,
	SDKHook_BlockedPost,
	SDKHook_OnTakeDamage_Alive,
	SDKHook_OnTakeDamage_AlivePost,
	SDKHook_CanBeAutobalanced,
	SDKHook_MAXHOOKS
};

extern HookTypeData g_HookTypes[SDKHook_MAXHOOKS];

enum HookReturn
{
	HookRet_Successful,
	HookRet_InvalidEntity,
	HookRet_InvalidHookType,
	HookRet_NotSupported,
	HookRet_BadEntForHookType,
};

#if SOURCE_ENGINE >= SE_CSS
typedef void *(*ReticulateSplines)();
#endif

/**
 * Classes
 */

class IPhysicsObject;
class CDmgAccumulator;
typedef CBaseEntity CBaseCombatWeapon;

namespace SourceMod {
	class IBinTools;
}

struct HookList
{
public:
	int entity;
	IPluginFunction *callback;
};

class CVTableList
{
public:
	CVTableList() : vtablehook(NULL)
	{
	};

	~CVTableList()
	{
		delete vtablehook;
	};

	CVTableList(const CVTableList&) = delete;
	CVTableList& operator= (const CVTableList&) = delete;
public:
	CVTableHook *vtablehook;
	std::vector<HookList> hooks;
};

class IEntityListener
{
public:
#if SOURCE_ENGINE == SE_BMS
	virtual ~IEntityListener() {};
	virtual void OnEntityPreSpawned( CBaseEntity *pEntity ) {};
#endif
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {};
	virtual void OnEntitySpawned( CBaseEntity *pEntity ) {};
	virtual void OnEntityDeleted( CBaseEntity *pEntity ) {};
#if SOURCE_ENGINE == SE_BMS
    virtual void OnEntityFlagsChanged( CBaseEntity *pEntity, int nAddedFlags, int nRemovedFlags ) {};
#endif
};

class SDKHooks :
	public SDKExtension,
	public IConCommandBaseAccessor,
	public IPluginsListener,
	public IFeatureProvider,
	public IEntityListener,
	public IClientListener,
	public ISDKHooks
{
public:
	SDKHooks();

	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	virtual bool QueryRunning(char *error, size_t maxlength);

	virtual bool QueryInterfaceDrop(SMInterface* pInterface);

	/** Returns version string */
	virtual const char *GetExtensionVerString();

	/** Returns date string */
	virtual const char *GetExtensionDateString();

public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength);
#endif

public:  // IPluginsListener
	virtual void OnPluginLoaded(IPlugin *plugin);
	virtual void OnPluginUnloaded(IPlugin *plugin);

public:  // IConCommandBaseAccessor
	virtual bool RegisterConCommandBase(ConCommandBase *pVar);

public:  // IFeatureProvider
	virtual FeatureStatus GetFeatureStatus(FeatureType type, const char *name);

public:  // IEntityListener
	virtual void OnEntityCreated(CBaseEntity *pEntity);
	virtual void OnEntityDeleted(CBaseEntity *pEntity);

public:  // IClientListener
	virtual void OnClientPutInServer(int client);
	virtual void OnClientDisconnecting(int client);

public:  // ISDKHooks
	virtual void AddEntityListener(ISMEntityListener *listener);
	virtual void RemoveEntityListener(ISMEntityListener *listener);

public:	// IServerGameDLL
	KHook::Return<void> LevelShutdown(IServerGameDLL*);
	KHook::Virtual<IServerGameDLL, void> m_HookLevelShutdown;
private:
	std::list<ISMEntityListener *> m_EntListeners;

public:
	/**
	 * Functions
	 */
	cell_t Call(int entity, SDKHookType type, int other=INVALID_EHANDLE_INDEX);
	cell_t Call(CBaseEntity *pEnt, SDKHookType type, int other=INVALID_EHANDLE_INDEX);
	cell_t Call(CBaseEntity *pEnt, SDKHookType type, CBaseEntity *pOther);
	void SetupHooks();

	HookReturn Hook(int entity, SDKHookType type, IPluginFunction *pCallback);
	void Unhook(int entity, SDKHookType type, IPluginFunction *pCallback);

	/**
	 * IServerGameDLL & IVEngineServer Hook Handlers
	 */
#ifdef GAMEDESC_CAN_CHANGE
	KHook::Return<const char*> Hook_GetGameDescription(IServerGameDLL*);
	KHook::Virtual<IServerGameDLL, const char*> m_HookGetGameDescription;
#endif
	KHook::Return<bool> Hook_LevelInit(IServerGameDLL*, char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	KHook::Virtual<IServerGameDLL, bool, char const*, char const*, char const*, char const*, bool, bool> m_HookLevelInit;

	/**
	 * CBaseEntity Hook Handlers
	 */
	KHook::Return<bool> Hook_CanBeAutobalanced(CBaseEntity*);
	KHook::Return<void> Hook_EndTouch(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_EndTouchPost(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_FireBulletsPost(CBaseEntity*, const FireBulletsInfo_t &info);
#ifdef GETMAXHEALTH_IS_VIRTUAL
	KHook::Return<int> Hook_GetMaxHealth(CBaseEntity*);
#endif
	KHook::Return<void> Hook_GroundEntChangedPost(CBaseEntity*, void *pVar);
	KHook::Return<int> Hook_OnTakeDamage(CBaseEntity*, CTakeDamageInfoHack &info);
	KHook::Return<int> Hook_OnTakeDamagePost(CBaseEntity*, CTakeDamageInfoHack &info);
	KHook::Return<int> Hook_OnTakeDamage_Alive(CBaseEntity*, CTakeDamageInfoHack &info);
	KHook::Return<int> Hook_OnTakeDamage_AlivePost(CBaseEntity*, CTakeDamageInfoHack &info);
	KHook::Return<void> Hook_PreThink(CBaseEntity*);
	KHook::Return<void> Hook_PreThinkPost(CBaseEntity*);
	KHook::Return<void> Hook_PostThink(CBaseEntity*);
	KHook::Return<void> Hook_PostThinkPost(CBaseEntity*);
	KHook::Return<bool> Hook_Reload(CBaseEntity*);
	KHook::Return<bool> Hook_ReloadPost(CBaseEntity*);
	KHook::Return<void> Hook_SetTransmit(CBaseEntity*, CCheckTransmitInfo *pInfo, bool bAlways);
	KHook::Return<bool> Hook_ShouldCollide(CBaseEntity*, int collisonGroup, int contentsMask);
	KHook::Return<void> Hook_Spawn(CBaseEntity*);
	KHook::Return<void> Hook_SpawnPost(CBaseEntity*);
	KHook::Return<void> Hook_StartTouch(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_StartTouchPost(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_Think(CBaseEntity*);
	KHook::Return<void> Hook_ThinkPost(CBaseEntity*);
	KHook::Return<void> Hook_Touch(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_TouchPost(CBaseEntity*, CBaseEntity *pOther);
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_PVKII
	KHook::Return<void> Hook_TraceAttack(CBaseEntity*, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);
	KHook::Return<void> Hook_TraceAttackPost(CBaseEntity*, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);
#else
	KHook::Return<void> Hook_TraceAttack(CBaseEntity*, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr);
	KHook::Return<void> Hook_TraceAttackPost(CBaseEntity*, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr);
#endif
	KHook::Return<void> Hook_Use(CBaseEntity*, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	KHook::Return<void> Hook_UsePost(CBaseEntity*, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	KHook::Return<void> Hook_VPhysicsUpdate(CBaseEntity*, IPhysicsObject *pPhysics);
	KHook::Return<void> Hook_VPhysicsUpdatePost(CBaseEntity*, IPhysicsObject *pPhysics);
	KHook::Return<void> Hook_Blocked(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<void> Hook_BlockedPost(CBaseEntity*, CBaseEntity *pOther);
	KHook::Return<bool> Hook_WeaponCanSwitchTo(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<bool> Hook_WeaponCanSwitchToPost(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<bool> Hook_WeaponCanUse(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<bool> Hook_WeaponCanUsePost(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<void> Hook_WeaponDrop(CBaseEntity*, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity);
	KHook::Return<void> Hook_WeaponDropPost(CBaseEntity*, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity);
	KHook::Return<void> Hook_WeaponEquip(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<void> Hook_WeaponEquipPost(CBaseEntity*, CBaseCombatWeapon *pWeapon);
	KHook::Return<bool> Hook_WeaponSwitch(CBaseEntity*, CBaseCombatWeapon *pWeapon, int viewmodelindex);
	KHook::Return<bool> Hook_WeaponSwitchPost(CBaseEntity*, CBaseCombatWeapon *pWeapon, int viewmodelindex);
	
private:
	void HandleEntityCreated(CBaseEntity *pEntity, int index, cell_t ref);
	void HandleEntityDeleted(CBaseEntity *pEntity);
	void Unhook(CBaseEntity *pEntity);
	void Unhook(IPluginContext *pContext);

private:
	KHook::Return<int> HandleOnTakeDamageHook(CBaseEntity*, CTakeDamageInfoHack &info, SDKHookType hookType);
	KHook::Return<int> HandleOnTakeDamageHookPost(CBaseEntity*, CTakeDamageInfoHack &info, SDKHookType hookType);

private:
	inline bool IsEntityIndexInRange(int i) { return i >= 0 && i < NUM_ENT_ENTRIES; }
	cell_t m_EntityCache[NUM_ENT_ENTRIES];
};

extern CGlobalVars *gpGlobals;
extern std::vector<CVTableList *> g_HookList[SDKHook_MAXHOOKS];

extern ICvar *icvar;

#if SOURCE_ENGINE >= SE_ORANGEBOX
extern IServerTools *servertools;
#endif
extern SourceMod::IBinTools *g_pBinTools;
extern IGameConfig *g_pGameConf;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
