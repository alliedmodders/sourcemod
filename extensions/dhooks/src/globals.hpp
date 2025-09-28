#pragma once

#include <IHandleSys.h>
#include <IExtensionSys.h>
#include <IGameHelpers.h>
#include <ISourceMod.h>
#include <sp_vm_types.h>

#include <thread>
#include <vector>

namespace dhooks {
class SignatureGameConfig;
};

namespace dhooks::globals {

extern std::thread::id main_thread;
extern SourceMod::IHandleSys* handlesys;
extern SourceMod::IGameHelpers* gamehelpers;
extern SourceMod::IExtension* myself;
extern SourceMod::ISourceMod* sourcemod;
extern std::vector<sp_nativeinfo_t> natives;
extern dhooks::SignatureGameConfig* dhooks_config;

void init();

inline const char *HandleErrorToString(SourceMod::HandleError err) {
	switch(err) {
		case SourceMod::HandleError_None: { return "No error"; }
		case SourceMod::HandleError_Changed: { return "The handle has been freed and reassigned"; }
		case SourceMod::HandleError_Type: { return "The handle has a different type registered"; }
		case SourceMod::HandleError_Freed: { return "The handle has been freed"; }
		case SourceMod::HandleError_Index: { return "generic internal indexing error"; }
		case SourceMod::HandleError_Access: { return "No access permitted to free this handle"; }
		case SourceMod::HandleError_Limit: { return "The limited number of handles has been reached"; }
		case SourceMod::HandleError_Identity: { return "The identity token was not usable"; }
		case SourceMod::HandleError_Owner: { return "Owners do not match for this operation"; }
		case SourceMod::HandleError_Version: { return "Unrecognized security structure version"; }
		case SourceMod::HandleError_Parameter: { return "An invalid parameter was passed"; }
		case SourceMod::HandleError_NoInherit: { return "This type cannot be inherited"; }
	}
	return "";
}

}