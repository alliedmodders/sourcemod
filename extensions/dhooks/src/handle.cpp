#include "handle.hpp"
#include "globals.hpp"

namespace dhooks::globals {
SourceMod::HandleType_t param_return_handle;
}

namespace dhooks::handle {

void init() {
	SourceMod::HandleAccess security;
	globals::handlesys->InitAccessDefaults(nullptr, &security);
	// Do not allow cloning, the struct self-manage its handle
	security.access[SourceMod::HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	globals::param_return_handle = globals::handlesys->CreateType("DHookParamReturn", nullptr, 0, nullptr, &security, globals::myself->GetIdentity(), nullptr);
}

ParamReturn::ParamReturn(Capsule* capsule, GeneralRegister* generalregs, FloatRegister* floatregs) :
	_handle(globals::handlesys->CreateHandle(globals::param_return_handle, this, globals::myself->GetIdentity(), globals::myself->GetIdentity(), nullptr)),
	_capsule(capsule),
	_general_registers(generalregs),
	_float_registers(floatregs) {
}

ParamReturn::~ParamReturn() {
	if (_handle != BAD_HANDLE) {
		globals::handlesys->FreeHandle(_handle, nullptr);
	}
}

}