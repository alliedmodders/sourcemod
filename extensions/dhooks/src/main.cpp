#include "globals.hpp"

#include <smsdk_ext.h>

class ExtensionBridge : public SDKExtension {
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late) override {
		if (dhooks::globals::init(g_SMAPI, myself, g_pShareSys, error, maxlength, late) == false) {
			return false;
		}
		return true;
	}
};

ExtensionBridge extension;
SMEXT_LINK(&extension);