#pragma once

#include "register.hpp"

#include <sp_vm_types.h>

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

	ParamReturn(Capsule*, GeneralRegister*, FloatRegister*);
	~ParamReturn();
private:
	cell_t _handle;
	Capsule* _capsule;
	GeneralRegister* _general_registers;
	FloatRegister* _float_registers;
};

};