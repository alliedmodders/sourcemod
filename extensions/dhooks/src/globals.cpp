#include "globals.hpp"
#include "signatures.hpp"

namespace dhooks::globals {

std::thread::id main_thread;
SourceMod::IExtension* myself;
SourceMod::IShareSys* sharedsys;
SourceMod::IHandleSys* handlesys;
SourceMod::IGameHelpers* gamehelpers;
SourceMod::ISourceMod* sourcemod;
SourceMod::IGameConfigManager* gameconfs;
dhooks::SignatureGameConfig dhooks_config;

template<typename Interface>
static inline Interface* RequestSourcemodInterface(
	const char* name,
	unsigned int version,
	char *error,
	size_t maxlength) {
	SourceMod::SMInterface* interface = nullptr;
	if (globals::sharedsys->RequestInterface(name, version, globals::myself, &interface) && interface != nullptr) {
		return (Interface*)interface;
	}

	size_t len = ke::SafeSprintf(error, maxlength, "Could not find interface: %s", name);
	if (len >= maxlength) {
		error[maxlength - 1] = '\0';
	}
	return nullptr;
}

bool init(SourceMM::ISmmAPI* smmapi, SourceMod::IExtension* me, SourceMod::IShareSys* sys, char *error, size_t maxlength, bool late) {
	globals::main_thread = std::this_thread::get_id();
	globals::myself = me;
	globals::sharedsys = sys;

	if ((globals::handlesys = RequestSourcemodInterface<SourceMod::IHandleSys>(
		SMINTERFACE_HANDLESYSTEM_NAME,
		SMINTERFACE_HANDLESYSTEM_VERSION,
		error,
		maxlength)) == nullptr) {
		return false;
	}

	if ((globals::gamehelpers = RequestSourcemodInterface<SourceMod::IGameHelpers>(
		SMINTERFACE_GAMEHELPERS_NAME,
		SMINTERFACE_GAMEHELPERS_VERSION,
		error,
		maxlength)) == nullptr) {
		return false;
	}

	if ((globals::sourcemod = RequestSourcemodInterface<SourceMod::ISourceMod>(
		SMINTERFACE_SOURCEMOD_NAME,
		SMINTERFACE_SOURCEMOD_VERSION,
		error,
		maxlength)) == nullptr) {
		return false;
	}

	if ((globals::gameconfs = RequestSourcemodInterface<SourceMod::IGameConfigManager>(
		SMINTERFACE_GAMECONFIG_NAME,
		SMINTERFACE_GAMECONFIG_VERSION,
		error,
		maxlength)) == nullptr) {
		return false;
	}

	return true;
}

}