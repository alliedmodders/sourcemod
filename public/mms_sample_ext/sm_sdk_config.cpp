#include "sm_sdk_config.h"

using namespace SourceMod;

bool SM_AcquireInterfaces(char *error, size_t maxlength)
{
	SM_FIND_IFACE_OR_FAIL(SOURCEMOD, sm_main, error, maxlength);

#if defined SMEXT_ENABLE_FORWARDSYS
	SM_FIND_IFACE_OR_FAIL(FORWARDMANAGER, sm_forwards, error, maxlength);
#endif
#if defined SMEXT_ENABLE_HANDLESYS
	SM_FIND_IFACE_OR_FAIL(HANDLESYSTEM, sm_handlesys, error, maxlength);
#endif
#if defined SMEXT_ENABLE_PLAYERHELPERS
	SM_FIND_IFACE_OR_FAIL(PLAYERMANAGER, sm_players, error, maxlength);
#endif
#if defined SMEXT_ENABLE_DBMANAGER
	SM_FIND_IFACE_OR_FAIL(DBI, sm_dbi, error, maxlength);
#endif 
#if defined SMEXT_ENABLE_GAMECONF
	SM_FIND_IFACE_OR_FAIL(GAMECONFIG, sm_gameconfs, error, maxlength);
#endif
#if defined SMEXT_ENABLE_MEMUTILS
	SM_FIND_IFACE_OR_FAIL(MEMORYUTILS, sm_memutils, error, maxlength);
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
	SM_FIND_IFACE_OR_FAIL(GAMEHELPERS, sm_gamehelpers, error, maxlength);
#endif
#if defined SMEXT_ENABLE_TIMERSYS
	SM_FIND_IFACE_OR_FAIL(TIMERSYS, sm_timersys, error, maxlength);
#endif
#if defined SMEXT_ENABLE_THREADER
	SM_FIND_IFACE_OR_FAIL(THREADER, sm_threader, error, maxlength);
#endif
#if defined SMEXT_ENABLE_LIBSYS
	SM_FIND_IFACE_OR_FAIL(LIBRARYSYS, sm_libsys, error, maxlength);
#endif
#if defined SMEXT_ENABLE_PLUGINSYS
	SM_FIND_IFACE_OR_FAIL(PLUGINSYSTEM, sm_plsys, error, maxlength);
#endif 
#if defined SMEXT_ENABLE_MENUS
	SM_FIND_IFACE_OR_FAIL(MENUMANAGER, sm_menus, error, maxlength);
#endif
#if defined SMEXT_ENABLE_ADMINSYS
	SM_FIND_IFACE_OR_FAIL(ADMINSYS, sm_adminsys, error, maxlength);
#endif
#if defined SMEXT_ENABLE_TEXTPARSERS
	SM_FIND_IFACE_OR_FAIL(TEXTPARSERS, sm_text, error, maxlength);
#endif

	return true;
}

void SM_UnsetInterfaces()
{
	myself = NULL;
	smexts = NULL;
	sharesys = NULL;
	sm_main = NULL;
#if defined SMEXT_ENABLE_FORWARDSYS
	sm_forwards = NULL;
#endif
#if defined SMEXT_ENABLE_HANDLESYS
	sm_handlesys = NULL;
#endif
#if defined SMEXT_ENABLE_PLAYERHELPERS
	sm_players = NULL;
#endif
#if defined SMEXT_ENABLE_DBMANAGER
	sm_dbi = NULL;
#endif 
#if defined SMEXT_ENABLE_GAMECONF
	sm_gameconfs = NULL;
#endif
#if defined SMEXT_ENABLE_MEMUTILS
	sm_memutils = NULL;
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
	sm_gamehelpers = NULL;
#endif
#if defined SMEXT_ENABLE_TIMERSYS
	sm_timersys = NULL;
#endif
#if defined SMEXT_ENABLE_THREADER
	sm_threader = NULL;
#endif
#if defined SMEXT_ENABLE_LIBSYS
	sm_libsys = NULL;
#endif
#if defined SMEXT_ENABLE_PLUGINSYS
	sm_plsys = NULL;
#endif 
#if defined SMEXT_ENABLE_MENUS
	sm_menus = NULL;
#endif
#if defined SMEXT_ENABLE_ADMINSYS
	sm_adminsys = NULL;
#endif
#if defined SMEXT_ENABLE_TEXTPARSERS
	sm_text = NULL;
#endif
}

IExtension *myself = NULL;
IExtensionManager *smexts = NULL;
IShareSys *sharesys = NULL;
SourceMod::ISourceMod *sm_main = NULL;
#if defined SMEXT_ENABLE_FORWARDSYS
SourceMod::IForwardManager *sm_forwards = NULL;
#endif
#if defined SMEXT_ENABLE_HANDLESYS
SourceMod::IHandleSys *sm_handlesys = NULL;
#endif
#if defined SMEXT_ENABLE_PLAYERHELPERS
SourceMod::IPlayerManager *sm_players = NULL;
#endif
#if defined SMEXT_ENABLE_DBMANAGER
SourceMod::IDBManager *sm_dbi = NULL;
#endif 
#if defined SMEXT_ENABLE_GAMECONF
SourceMod::IGameConfigManager *sm_gameconfs = NULL;
#endif
#if defined SMEXT_ENABLE_MEMUTILS
SourceMod::IMemoryUtils *sm_memutils = NULL;
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
SourceMod::IGameHelpers *sm_gamehelpers = NULL;
#endif
#if defined SMEXT_ENABLE_TIMERSYS
SourceMod::ITimerSystem *sm_timersys = NULL;
#endif
#if defined SMEXT_ENABLE_THREADER
SourceMod::IThreader *sm_threader = NULL;
#endif
#if defined SMEXT_ENABLE_LIBSYS
SourceMod::ILibrarySys *sm_libsys = NULL;
#endif
#if defined SMEXT_ENABLE_PLUGINSYS
SourceMod::IPluginManager *sm_plsys = NULL;
#endif 
#if defined SMEXT_ENABLE_MENUS
SourceMod::IMenuManager *sm_menus = NULL;
#endif
#if defined SMEXT_ENABLE_ADMINSYS
SourceMod::IAdminSystem *sm_adminsys = NULL;
#endif
#if defined SMEXT_ENABLE_TEXTPARSERS
SourceMod::ITextParsers *sm_text = NULL;
#endif

