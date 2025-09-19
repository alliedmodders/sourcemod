#include "handle.hpp"
#include "globals.hpp"
#include "capsule.hpp"

#include <cstdlib>

namespace dhooks::handle {

SourceMod::HandleType_t ParamReturn::HANDLE_TYPE = 0;

ParamReturn::ParamReturn(const Capsule* capsule, GeneralRegister* generalregs, FloatRegister* floatregs, void* return_ptr) :
	_handle(globals::handlesys->CreateHandle(handle::ParamReturn::HANDLE_TYPE, this, globals::myself->GetIdentity(), globals::myself->GetIdentity(), nullptr)),
	_capsule(capsule),
	_general_registers(generalregs),
	_float_registers(floatregs),
	_return(return_ptr) {
}

ParamReturn::~ParamReturn() {
	if (_handle != BAD_HANDLE) {
		globals::handlesys->FreeHandle(_handle, nullptr);
	}
}

// This is a helper function for very trivial types (that are held by only one register)
// If dhooks becomes more complex this logic must be entirely re-written
void* ParamReturn::_Get(size_t index) const {
	const auto& params = _capsule->GetParameters();
	if (params.size() <= index) {
		// What the hell
		std::abort();
	}

	const auto& variable = params[index];
	if (variable.reg_index) {
		AsmRegCode code = variable.reg_index.value();

		if (variable.reg_offset) {
			// The register already acts as a pointer
			return reinterpret_cast<void*>(static_cast<std::uintptr_t>(_general_registers[code]) + variable.reg_offset.value());
		} else {
			// The register holds the value, return a pointer to it
			return reinterpret_cast<void*>(&_general_registers[code]);
		}
	} else if (variable.float_reg_index) {
		AsmFloatRegCode code = variable.float_reg_index.value();

		if (variable.reg_offset) {
			// Floating registers can't be pointers
			std::abort();
		} else {
			// The register holds the value, return a pointer to it
			return reinterpret_cast<void*>(&_float_registers[code]);
		}
	} else {
		// What the hell again
		std::abort();
	}
}

void init() {
	SourceMod::HandleAccess security;
	globals::handlesys->InitAccessDefaults(nullptr, &security);
	// Do not allow cloning, the struct self-manage its handle
	security.access[SourceMod::HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	ParamReturn::HANDLE_TYPE = globals::handlesys->CreateType("DHookParamReturn", nullptr, 0, nullptr, &security, globals::myself->GetIdentity(), nullptr);
}

}