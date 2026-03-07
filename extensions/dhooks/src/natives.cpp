#include "natives.hpp"
#include "natives/dhookparam.hpp"
#include "natives/dhookreturn.hpp"
#include "natives/dhooksetup.hpp"
#include "natives/dynamicdetour.hpp"
#include "natives/dynamichook.hpp"

namespace dhooks::natives {
void init(std::vector<sp_nativeinfo_t>& natives) {
	dhookparam::init(natives);
	dhookreturn::init(natives);
	dhooksetup::init(natives);
	dynamicdetour::init(natives);
	dynamichook::init(natives);
}
}