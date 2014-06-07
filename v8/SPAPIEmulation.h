#ifndef _INCLUDE_V8_SPAPIEMULATION_H_
#define _INCLUDE_V8_SPAPIEMULATION_H_

#include <sp_vm_api.h>
#include <vector>
#include <list>
#include <cstdarg>
#include <v8.h>
#include "ScriptLoader.h"
#include "Require/RequireManager.h"
#include "V8Manager.h"

namespace SMV8
{
	namespace SPEmulation
	{
		using namespace SourcePawn;
		using namespace v8;

		enum CellType
		{
			INT = 0,
			FLOAT = 1,
			BOOL = 2,
			INTBYREF = 3,
			FLOATBYREF = 4,
			BOOLBYREF = 5,
			ARRAY = 6,
			STRING = 7,
			VARARG = 8
		};

		class PluginRuntime;
		class PluginContext;
		class PluginFunction;

		struct NativeParamInfo
		{
			NativeParamInfo(std::string name, CellType type) : name(name), type(type)
			{}
			std::string name;
			CellType type;
		};

		struct NativeData
		{
			std::string name;
			std::vector<NativeParamInfo> params;
			sp_native_t state;
			PluginRuntime* runtime;
			CellType resultType;
		};

		struct PubvarData
		{
			cell_t local_addr;
			sp_pubvar_t pubvar;
		};

		struct PublicData
		{
			std::string name;
			Persistent<Function> func;
			sp_public_t state;
			PluginFunction* pfunc;
			PluginRuntime *runtime;
		};

		struct SPToV8ReferenceInfo
		{
			Persistent<Object> refObj;
			void* addr;
			size_t size;
		};

		class PluginFunction : public IPluginFunction
		{
		public:
			PluginFunction(PluginRuntime& ctx, funcid_t id);
			virtual ~PluginFunction();
			virtual int PushCell(cell_t cell);
			virtual int PushCellByRef(cell_t *cell, int flags=SM_PARAM_COPYBACK);
			virtual int PushFloat(float number);
			virtual int PushFloatByRef(float *number, int flags=SM_PARAM_COPYBACK);
			virtual int PushArray(cell_t *inarray, unsigned int cells, int flags=0);
			virtual int PushString(const char *string);
			virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
			virtual void Cancel();
			virtual int Execute(cell_t *result);
			virtual int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result);
			virtual IPluginContext *GetParentContext();
			virtual bool IsRunnable();
			virtual funcid_t GetFunctionID();
			virtual int Execute2(IPluginContext *ctx, cell_t *result);
			virtual int CallFunction2(IPluginContext *ctx, 
				const cell_t *params, 
				unsigned int num_params, 
				cell_t *result);
			virtual IPluginRuntime *GetParentRuntime();
			virtual int PushValue(Handle<Value> val);
			virtual void SetSingleCellValue(Handle<Number> val, cell_t *result);
			virtual void CopyBackString(Handle<String> val, SPToV8ReferenceInfo *ref);
			virtual void CopyBackArray(Handle<Array> val, SPToV8ReferenceInfo *ref);
			virtual void CopyBackRefs();
			virtual Handle<Object> MakeRefObj(Handle<Value> val, void *addr, size_t size, bool copyback);
		private:
			PluginRuntime& runtime;
			funcid_t id;
			Persistent<Value> params[SP_MAX_EXEC_PARAMS];
			std::vector<SPToV8ReferenceInfo*> refs;
			int curParam;
		};

		/* Bridges between the SPAPI and V8 implementation by holding an internal stack only used during 
		   native or cross-plugin calls */
		class PluginContext : public IPluginContext
		{
		public:
			PluginContext(PluginRuntime* parentRuntime);
			virtual ~PluginContext();
		public:
			virtual IVirtualMachine *GetVirtualMachine();
			virtual sp_context_t *GetContext();
			virtual bool IsDebugging();
			virtual int SetDebugBreak(void *newpfn, void *oldpfn);
			virtual IPluginDebugInfo *GetDebugInfo();
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
			virtual int StringToLocal(cell_t local_addr, size_t bytes, const char *source);
			virtual int StringToLocalUTF8(cell_t local_addr, 
										  size_t maxbytes, 
										  const char *source, 
										  size_t *wrtnbytes);

			virtual int PushCell(cell_t value);
			virtual int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells);
			virtual int PushString(cell_t *local_addr, char **phys_addr, const char *string);
			virtual int PushCellsFromArray(cell_t array[], unsigned int numcells);
			virtual int BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite);
			virtual int BindNative(const sp_nativeinfo_t *native);
			virtual int BindNativeToAny(SPVM_NATIVE_FUNC native);
			virtual int Execute(uint32_t code_addr, cell_t *result);
			virtual cell_t ThrowNativeErrorEx(int error, const char *msg, ...);
			virtual cell_t ThrowNativeError(const char *msg, ...);
			virtual cell_t ThrowNativeErrorEx(int error, const char *msg, va_list args);
			virtual cell_t ThrowNativeError(const char *msg, va_list args);
			virtual IPluginFunction *GetFunctionByName(const char *public_name);
			virtual IPluginFunction *GetFunctionById(funcid_t func_id);
			virtual SourceMod::IdentityToken_t *GetIdentity();
			virtual cell_t *GetNullRef(SP_NULL_TYPE type);
			virtual int LocalToStringNULL(cell_t local_addr, char **addr);
			virtual int BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC native);
			virtual bool IsInExec();
			virtual IPluginRuntime *GetRuntime();
			virtual int Execute2(IPluginFunction *function, 
				const cell_t *params, 
				unsigned int num_params, 
				cell_t *result);
			Handle<Value> ExecuteV8(IPluginFunction *function, int argc, Handle<Value> argv[]);
			virtual int GetLastNativeError();
			virtual cell_t *GetLocalParams();
			virtual void SetKey(int k, void *value);
			virtual bool GetKey(int k, void **value);
			virtual void ClearLastNativeError();
			virtual void CheckHeapSize(unsigned int cells);
		private:
			PluginRuntime *parentRuntime;
			int32_t nativeError;
			void *keys[4]; // <-- What... the... crap...
			bool keys_set[4]; // <-- What... the... crap...
			char* heap;
			char* hp;
			unsigned int heapSize;
			bool inExec;
			std::string errMessage;
		};

		class PluginDebugInfo : public IPluginDebugInfo
		{
		public:
			virtual int LookupFile(ucell_t addr, const char **filename);
			virtual int LookupFunction(ucell_t addr, const char **name);
			virtual int LookupLine(ucell_t addr, uint32_t *line);
		};

		class Compilation : public ICompilation
		{
		public:
			virtual bool SetOption(const char *key, const char *val);
			virtual void Abort();
		};

		class PluginRuntime : public IPluginRuntime
		{
			friend class PluginFunction;
		public:
			PluginRuntime(Isolate* isolate, Manager *manager, Require::RequireManager *reqMan, ScriptLoader *script_loader, SMV8Script plugin_script);
			virtual ~PluginRuntime();
			virtual IPluginDebugInfo *GetDebugInfo();
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
			virtual IPluginFunction *GetFunctionByName(const char *public_name);
			virtual IPluginFunction *GetFunctionById(funcid_t func_id);
			virtual IPluginContext *GetDefaultContext();
			virtual bool IsDebugging();
			virtual int ApplyCompilationOptions(ICompilation *co);
			virtual void SetPauseState(bool paused);
			virtual bool IsPaused();
			virtual size_t GetMemUsage();
			virtual unsigned char *GetCodeHash();
			virtual unsigned char *GetDataHash();
			virtual Handle<Value> CallV8Function(funcid_t func, int argc, Handle<Value> argv[]);
			virtual Isolate* GetIsolate();
			virtual void ExtractForwards();
			virtual funcid_t MakeVolatilePublic(Handle<Function> func);
		protected:
			virtual Handle<ObjectTemplate> GenerateGlobalObjectTemplate();
			virtual Handle<ObjectTemplate> GenerateNativesObjectTemplate();
			virtual Handle<ObjectTemplate> GeneratePluginObjectTemplate();
			virtual Handle<ObjectTemplate> GeneratePluginInfoObjectTemplate();
			static void DeclareNative(const FunctionCallbackInfo<Value>& info);
//			virtual void InsertNativeParams(NativeData& nd, Handle<Array> signature);
//			virtual NativeParamInfo CreateNativeParamInfo(Handle<Object> paramInfo);
			virtual void ExtractPluginInfo();
			virtual void LoadEmulatedString(const std::string& realstr, cell_t& local_addr_target);
			virtual void RegisterNativeInNativesObject(NativeData& native);
			static void NativeRouter(const FunctionCallbackInfo<Value>& info);
			virtual funcid_t AllocateVolatilePublic(PublicData *pd);
			static void VolatilePublicDisposer(Isolate* isolate, Persistent<Function> *func, PublicData* self);
			static void Require(const FunctionCallbackInfo<Value>& info);
			static void GetMaxClients(const FunctionCallbackInfo<Value>& info);
			virtual void GenerateMaxClients();
			static void RequireExt(const FunctionCallbackInfo<Value>& info);
		private:
			std::vector<NativeData*> natives;
			std::vector<PublicData*> publics;
			std::vector<PubvarData> pubvars;
			bool pauseState;
			Isolate* isolate;
			Persistent<Context> v8Context;
			PluginContext ctx;
			Persistent<Object> nativesObj;
			SMV8Script plugin_script;
			Require::RequireManager *reqMan;
			ScriptLoader *script_loader;
			Manager *manager;
			SMV8Script *current_script;
			cell_t *maxClients;
			std::list<std::string> loadedExts;
		};


		class ContextTrace : public IContextTrace
		{
		public:
			virtual int GetErrorCode();
			virtual const char *GetErrorString();
			virtual bool DebugInfoAvailable();
			virtual const char *GetCustomErrorString();
			virtual bool GetTraceInfo(CallStackInfo *trace);
			virtual void ResetTrace();
			virtual const char *GetLastNative(uint32_t *index);
		};

		class DebugListener : public IDebugListener
		{
		public:
			virtual void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error);
			virtual void OnDebugSpew(const char *msg, ...);
		};

		class Profiler : public IProfiler
		{
		public:
			virtual void OnNativeBegin(IPluginContext *pContext, sp_native_t *native);
			virtual void OnNativeEnd();
			virtual void OnFunctionBegin(IPluginContext *pContext, const char *name);
			virtual void OnFunctionEnd();
			virtual int OnCallbackBegin(IPluginContext *pContext, sp_public_t *pubfunc);
			virtual void OnCallbackEnd(int serial);
		};
	}
}

#endif // !defined _INCLUDE_V8_SPAPIEMULATION_H_
