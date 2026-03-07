#pragma once

#include "register.hpp"
#include "signatures.hpp"

#include <sp_vm_types.h>
#include <IHandleSys.h>

#include <optional>
#include <vector>
#include <unordered_map>

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
protected:
	HookSetup(
		dhooks::sp::ThisPointerType,
		dhooks::sp::CallingConvention,
		std::vector<dhooks::ArgumentInfo> const&,
		dhooks::ReturnInfo const&
	);
public:

	virtual ~HookSetup() = default;
	static SourceMod::HandleType_t HANDLE_TYPE;

	bool IsImmutable() const { return _immutable; }
	void SetImmutable() { _immutable = true; }

	void AddParam(const Variable& var) { if (!_immutable) { _dhook_params.push_back(var); } }
	cell_t GetHandle() const { return _handle; }

	const sp::CallingConvention GetCallConv() const { return _dhook_call_conv; }
	const std::vector<Variable>& GetParameters() const { return _dhook_params; }
	const ReturnVariable& GetReturn() const { return _dhook_return; }
protected:
	cell_t _handle;
	bool _immutable = false;
	sp::ThisPointerType _this_pointer;
	sp::CallingConvention _dhook_call_conv;
	std::vector<Variable> _dhook_params;
	ReturnVariable _dhook_return;
};

class DynamicHook : public HookSetup {
public:
	DynamicHook(
		SourceMod::IdentityToken_t*,
		sp::ThisPointerType,
		std::uint32_t,
		sp::HookType,
		const std::vector<ArgumentInfo>& params,
		const ReturnInfo& ret,
		SourcePawn::IPluginFunction*
	);
	static SourceMod::HandleType_t HANDLE_TYPE;

	void SetOffset(int offset) {
		if (offset < 0) {
			return;
		}
		_offset = offset;
	}

	sp::HookType GetType() const {
		return _hook_type;
	}

	SourcePawn::IPluginFunction* GetDefaultCallback() const {
		return _default_callback;
	}
protected:
	std::uint32_t _offset;
	sp::HookType _hook_type;
	SourcePawn::IPluginFunction* _default_callback;
};

class DynamicDetour : public HookSetup {
public:
	DynamicDetour(SourceMod::IdentityToken_t*, sp::ThisPointerType, sp::CallingConvention, void* address, const std::vector<ArgumentInfo>& params, const ReturnInfo& ret);
	static SourceMod::HandleType_t HANDLE_TYPE;

	void SetAddress(void* address) {
		if (address == nullptr) {
			return;
		}
		_address = address;
	}

	void* GetAddress() const {
		return _address;
	}

	bool Enable(SourcePawn::IPluginFunction*, sp::HookMode);
	bool Disable(SourcePawn::IPluginFunction*, sp::HookMode);
protected:
	void* _address;
	std::unordered_map<SourcePawn::IPluginFunction*, std::uint32_t> _pre_detours;
	std::unordered_map<SourcePawn::IPluginFunction*, std::uint32_t> _post_detours;
};

};