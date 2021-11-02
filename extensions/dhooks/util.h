#ifndef _INCLUDE_UTIL_FUNCTIONS_H_
#define _INCLUDE_UTIL_FUNCTIONS_H_

#include "vhook.h"
#include "convention.h"

enum PluginRegister
{
	// Don't change the register and use the default for the calling convention.
	DHookRegister_Default,

	// 8-bit general purpose registers
	DHookRegister_AL,
	DHookRegister_CL,
	DHookRegister_DL,
	DHookRegister_BL,
	DHookRegister_AH,
	DHookRegister_CH,
	DHookRegister_DH,
	DHookRegister_BH,

	// 32-bit general purpose registers
	DHookRegister_EAX,
	DHookRegister_ECX,
	DHookRegister_EDX,
	DHookRegister_EBX,
	DHookRegister_ESP,
	DHookRegister_EBP,
	DHookRegister_ESI,
	DHookRegister_EDI,

	// 128-bit XMM registers
	DHookRegister_XMM0,
	DHookRegister_XMM1,
	DHookRegister_XMM2,
	DHookRegister_XMM3,
	DHookRegister_XMM4,
	DHookRegister_XMM5,
	DHookRegister_XMM6,
	DHookRegister_XMM7,

	// 80-bit FPU registers
	DHookRegister_ST0
};

size_t GetParamOffset(HookParamsStruct *params, unsigned int index);
void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset);
size_t GetParamTypeSize(HookParamType type);
size_t GetParamsSize(DHooksCallback *dg);

DataType_t DynamicHooks_ConvertParamTypeFrom(HookParamType type);
DataType_t DynamicHooks_ConvertReturnTypeFrom(ReturnType type);
Register_t DynamicHooks_ConvertRegisterFrom(PluginRegister reg);
#endif
