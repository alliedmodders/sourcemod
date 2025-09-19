#pragma once

#include "register.hpp"

#include <sp_vm_types.h>
#include <IHandleSys.h>

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

};