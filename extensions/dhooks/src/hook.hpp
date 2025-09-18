#pragma once

#include "sp_inc.hpp"

#include <sp_vm_api.h>
#include <optional>

namespace dhooks {

class Capsule;

struct HookCallback {
	sp::ThisPointerType this_pointer_type;
	std::optional<void*> associated_this;
	Capsule* associated_capsule;
	SourcePawn::IPluginFunction* callback;
};

}