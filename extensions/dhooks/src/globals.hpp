#pragma once

#include <IHandleSys.h>
#include <IExtensionSys.h>
#include <IGameHelpers.h>
#include <ISourceMod.h>

#include <thread>

namespace dhooks::globals {

extern std::thread::id main_thread;
extern SourceMod::IHandleSys* handlesys;
extern SourceMod::IGameHelpers* gamehelpers;
extern SourceMod::IExtension* myself;
extern SourceMod::ISourceMod* sourcemod;

void init();

}