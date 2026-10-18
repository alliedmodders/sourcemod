#pragma once

#include <sp_vm_types.h>
#include <ISDKHooks.h>

#include <vector>

namespace dhooks::natives::dhookentitylistener {
void init(std::vector<sp_nativeinfo_t>& natives);

void add_listener(SourceMod::ISDKHooks* sdkhooks);
void remove_listener(SourceMod::ISDKHooks* sdkhooks);
void remove_plugin(SourcePawn::IPluginContext* context);
}