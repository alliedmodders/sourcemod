#include "globals.hpp"
#include "natives.hpp"
#include "signatures.hpp"
#include "handle.hpp"

#include <smsdk_ext.h>

std::vector<sp_nativeinfo_t> gNatives;

class ExtensionBridge : public SDKExtension {
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late) override {
		if (dhooks::globals::init(g_SMAPI, myself, g_pShareSys, error, maxlength, late) == false) {
			return false;
		}
		dhooks::handle::init();

		gNatives.reserve(1000);
		dhooks::natives::init(gNatives);
		gNatives.push_back({nullptr, nullptr});
		sharesys->AddNatives(myself, gNatives.data());
		sharesys->AddDependency(myself, "sdktools.ext", true, true);

		dhooks::globals::gameconfs->AddUserConfigHook("Functions", &dhooks::globals::dhooks_config);
		return true;
	}

	virtual void SDK_OnAllLoaded() override {
		SM_GET_LATE_IFACE(SDKTOOLS, dhooks::globals::sdktools);
	}

	virtual bool QueryInterfaceDrop(SourceMod::SMInterface* interface) override {
		if (interface == dhooks::globals::sdktools) {
			dhooks::globals::sdktools = nullptr;
			return false;
		}

		return SDKExtension::QueryInterfaceDrop(interface);
	}
};

ExtensionBridge extension;
SMEXT_LINK(&extension);