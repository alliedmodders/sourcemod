#ifndef _INCLUDE_SIGNATURES_H_
#define _INCLUDE_SIGNATURES_H_

#include "extension.h"
#include "util.h"
#include <string>
#include <vector>
#include <sm_stringhashmap.h>

struct ArgumentInfo {
	ArgumentInfo() : name()
	{ }

	ArgumentInfo(std::string name, ParamInfo info) : name(name), info(info)
	{ }

	std::string name;
	ParamInfo info;
};

class SignatureWrapper {
public:
	std::string signature;
	std::string address;
	std::string offset;
	std::vector<ArgumentInfo> args;
	CallingConvention callConv;
	HookType hookType;
	ReturnType retType;
	ThisPointerType thisType;
};

class SignatureGameConfig : public ITextListener_SMC {
public:
	SignatureWrapper *GetFunctionSignature(const char *function);
public:
	//ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	void ReadSMC_ParseStart();

private:
	ReturnType GetReturnTypeFromString(const char *str);
	HookParamType GetHookParamTypeFromString(const char *str);
	Register_t GetCustomRegisterFromString(const char *str);

private:
	StringHashMap<SignatureWrapper *> signatures_;
};

extern SignatureGameConfig *g_pSignatures;
#endif
