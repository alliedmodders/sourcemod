#ifndef _INCLUDE_SOURCEPAWN_BASECONTEXT_H_
#define _INCLUDE_SOURCEPAWN_BASECONTEXT_H_

#include "sp_vm_context.h"

namespace SourcePawn
{
	class BaseContext : 
		public IPluginContext,
		public IPluginDebugInfo
	{
	public:
		BaseContext(sp_context_t *ctx);
	public: //IPluginContext
		IVirtualMachine *GetVirtualMachine();
		sp_context_t *GetContext();
		bool IsDebugging();
		int SetDebugBreak(SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn);
		IPluginDebugInfo *GetDebugInfo();
		virtual int HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr);
		virtual int HeapPop(cell_t local_addr);
		virtual int HeapRelease(cell_t local_addr);
		virtual int FindNativeByName(const char *name, uint32_t *index);
		virtual int GetNativeByIndex(uint32_t index, sp_native_t **native);
		virtual uint32_t GetNativesNum();
		virtual int FindPublicByName(const char *name, uint32_t *index);
		virtual int GetPublicByIndex(uint32_t index, sp_public_t **publicptr);
		virtual uint32_t GetPublicsNum();
		virtual int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar);
		virtual int FindPubvarByName(const char *name, uint32_t *index);
		virtual int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr);
		virtual uint32_t GetPubVarsNum();
		virtual int LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr);
		virtual int LocalToString(cell_t local_addr, char *buffer, size_t maxlength, int *chars);
		virtual int StringToLocal(cell_t local_addr, size_t chars, const char *source);
		virtual int PushCell(cell_t value);
		virtual int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells);
		virtual int PushString(cell_t *local_addr, cell_t **phys_addr, const char *string);
		virtual int PushCellsFromArray(cell_t array[], unsigned int numcells);
		virtual int BindNatives(sp_nativeinfo_t *natives, unsigned int num, int overwrite);
		virtual int BindNative(sp_nativeinfo_t *native, uint32_t status);
		virtual int BindNativeToAny(SPVM_NATIVE_FUNC native);
		virtual int Execute(uint32_t public_func, cell_t *result);
	public: //IPluginDebugInfo
		virtual int LookupFile(ucell_t addr, const char **filename);
		virtual int LookupFunction(ucell_t addr, const char **name);
		virtual int LookupLine(ucell_t addr, uint32_t *line);
	private:
		sp_context_t *ctx;
	};
};

#endif //_INCLUDE_SOURCEPAWN_BASECONTEXT_H_
