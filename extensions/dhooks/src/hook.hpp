#pragma once

#include "sp_inc.hpp"

#include <sp_vm_api.h>
#include <optional>

namespace dhooks {

class Capsule;

struct HookCallback {
	sp::ThisPointerType this_pointer_type;
	void* associated_this;
	Capsule* associated_capsule;
	SourcePawn::IPluginFunction* callback;
	SourcePawn::IPluginFunction* remove_callback;
	bool using_int64_address;
};

}