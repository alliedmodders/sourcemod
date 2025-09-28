#pragma once

#include "register.hpp"
#include "variable.hpp"

#include <sp_vm_types.h>
#include <IHandleSys.h>

#include <optional>
#include <vector>

namespace dhooks {
class Capsule;
}

namespace dhooks::handle {

void init();

class ParamReturn {
public:
	operator cell_t() const {
		return _handle;
	}
	operator cell_t() {
		return _handle;
	}

	ParamReturn(const Capsule*, GeneralRegister*, FloatRegister*, void*);
	~ParamReturn();

	template<typename T>
    inline T* Get(size_t index) const { return reinterpret_cast<T*>(_Get(index)); };
	template<typename T>
    inline T* GetReturn() const { return reinterpret_cast<T*>(_return); };
	inline const Capsule* GetCapsule() const { return _capsule; };

	static SourceMod::HandleType_t HANDLE_TYPE;
private:
	void* _Get(size_t index) const;

	cell_t _handle;
	const Capsule* _capsule;
	GeneralRegister* _general_registers;
	FloatRegister* _float_registers;
	void* _return;
};

class HookSetup {
public:
	HookSetup(sp::ThisPointerType, sp::CallingConvention, void* address, const std::vector<Variable>& params, const ReturnVariable& ret);
	HookSetup(sp::ThisPointerType, std::uint32_t offset, const std::vector<Variable>& params, const ReturnVariable& ret);
	~HookSetup();

	static SourceMod::HandleType_t VIRTUAL_HANDLE_TYPE;
	static SourceMod::HandleType_t ADDRESS_HANDLE_TYPE;
private:
	cell_t _handle;
	std::optional<std::uint32_t> _virtual_offset;
	std::optional<void*> _address;

	sp::ThisPointerType _this_pointer;
	sp::CallingConvention _dhook_call_conv;
	std::vector<Variable> _dhook_params;
	ReturnVariable _dhook_return;
};

};