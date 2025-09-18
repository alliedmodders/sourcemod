#pragma once

#include <IHandleSys.h>
#include <IExtensionSys.h>

#include <thread>

namespace dhooks::globals {

extern std::thread::id main_thread;
extern SourceMod::IHandleSys* handlesys;

extern SourceMod::IExtension* myself;

void init();

}