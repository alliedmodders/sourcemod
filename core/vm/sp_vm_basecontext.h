#ifndef _INCLUDE_SOURCEPAWN_BASECONTEXT_H_
#define _INCLUDE_SOURCEPAWN_BASECONTEXT_H_

#include "sp_vm_api.h"
#include "sp_vm_function.h"

/**
 * :TODO: Make functions allocate as a lump instead of individual allocations!
 */

namespace SourcePawn
{
	class BaseContext : 
		public IPluginContext,
		public IPluginDebugInfo
	{
	public:
		BaseContext(sp_context_t *ctx);
		~BaseContext();
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
		virtual int LocalToString(cell_t local_addr, char **addr);
		virtual int StringToLocal(cell_t local_addr, size_t chars, const char *source);
		virtual int StringToLocalUTF8(cell_t local_addr, size_t maxbytes, const char *source, size_t *wrtnbytes);
		virtual int PushCell(cell_t value);
		virtual int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells);
		virtual int PushString(cell_t *local_addr, char **phys_addr, const char *string);
		virtual int PushCellsFromArray(cell_t array[], unsigned int numcells);
		virtual int BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite);
		virtual int BindNative(const sp_nativeinfo_t *native);
		virtual int BindNativeToAny(SPVM_NATIVE_FUNC native);
		virtual int Execute(funcid_t funcid, cell_t *result);
		virtual void ThrowNativeErrorEx(int error, const char *msg, ...);
		virtual cell_t ThrowNativeError(const char *msg, ...);
		virtual IPluginFunction *GetFunctionByName(const char *public_name);
		virtual IPluginFunction *GetFunctionById(funcid_t func_id);
#if defined SOURCEMOD_BUILD
		virtual SourceMod::IdentityToken_t *GetIdentity();
		void SetIdentity(SourceMod::IdentityToken_t *token);
		bool IsRunnable();
		void SetRunnable(bool runnable);
#endif
	public: //IPluginDebugInfo
		virtual int LookupFile(ucell_t addr, const char **filename);
		virtual int LookupFunction(ucell_t addr, const char **name);
		virtual int LookupLine(ucell_t addr, uint32_t *line);
	public:
		void SetContext(sp_context_t *_ctx);
	private:
		void SetErrorMessage(const char *msg, va_list ap);
		void FlushFunctionCache();
	private:
		sp_context_t *ctx;
#if defined SOURCEMOD_BUILD
		SourceMod::IdentityToken_t *m_pToken;
#endif
		char m_MsgCache[1024];
		bool m_CustomMsg;
		bool m_InExec;
		bool m_Runnable;
		unsigned int m_funcsnum;
		CFunction **m_priv_funcs;
		CFunction **m_pub_funcs;
	};
};

#endif //_INCLUDE_SOURCEPAWN_BASECONTEXT_H_
