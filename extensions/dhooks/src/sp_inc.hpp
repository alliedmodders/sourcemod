#pragma once

namespace dhooks::sp {

enum CallingConvention
{
	CallConv_CDECL,
	CallConv_THISCALL,
	CallConv_STDCALL,
	CallConv_FASTCALL,
};

enum HookParamType
{
	HookParamType_Unknown,
	HookParamType_Int,
	HookParamType_Bool,
	HookParamType_Float,
	HookParamType_String,
	HookParamType_StringPtr,
	HookParamType_CharPtr,
	HookParamType_VectorPtr,
	HookParamType_CBaseEntity,
	HookParamType_ObjectPtr,
	HookParamType_Edict,
	HookParamType_Object
};

enum ReturnType
{
	ReturnType_Unknown,
	ReturnType_Void,
	ReturnType_Int,
	ReturnType_Bool,
	ReturnType_Float,
	ReturnType_String,
	ReturnType_StringPtr,
	ReturnType_CharPtr,
	ReturnType_Vector,
	ReturnType_VectorPtr,
	ReturnType_CBaseEntity,
	ReturnType_Edict
};

enum DHookPassFlag
{
	DHookPass_ByVal =     (1<<0),
	DHookPass_ByRef =     (1<<1),
	DHookPass_ODTOR =     (1<<2),
	DHookPass_OCTOR =     (1<<3),
	DHookPass_OASSIGNOP = (1<<4),
};

enum MRESReturn
{
	MRES_ChangedHandled = -2,
	MRES_ChangedOverride,
	MRES_Ignored,
	MRES_Handled,
	MRES_Override,
	MRES_Supercede
};

enum DHookRegister
{
	DHookRegister_Default,
	DHookRegister_AL,
	DHookRegister_CL,
	DHookRegister_DL,
	DHookRegister_BL,
	DHookRegister_AH,
	DHookRegister_CH,
	DHookRegister_DH,
	DHookRegister_BH,
	DHookRegister_EAX,
	DHookRegister_ECX,
	DHookRegister_EDX,
	DHookRegister_EBX,
	DHookRegister_ESP,
	DHookRegister_EBP,
	DHookRegister_ESI,
	DHookRegister_EDI,
	DHookRegister_XMM0,
	DHookRegister_XMM1,
	DHookRegister_XMM2,
	DHookRegister_XMM3,
	DHookRegister_XMM4,
	DHookRegister_XMM5,
	DHookRegister_XMM6,
	DHookRegister_XMM7,
	DHookRegister_ST0
};

}