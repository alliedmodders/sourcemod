#ifndef _INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_
#define _INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_

#include "sp_vm_types.h"

inline cell_t sp_ftoc(float val)
{
	return *(cell_t *)&val;
}
inline float sp_ctof(cell_t val)
{
	return *(float *)&val;
}

#endif //_INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_