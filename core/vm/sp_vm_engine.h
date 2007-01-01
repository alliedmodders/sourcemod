#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

#include "sp_vm_api.h"

namespace SourcePawn
{
	struct TracedCall
	{
		uint32_t cip;
		uint32_t frm;
		sp_context_t *ctx;
		TracedCall *next;
		unsigned int chain;
	};


	class CContextTrace : public IContextTrace
	{
	public:
		CContextTrace(TracedCall *pStart, int error, const char *msg, uint32_t native);
	public:
		virtual int GetErrorCode();
		virtual const char *GetErrorString();
		virtual bool DebugInfoAvailable();
		virtual const char *GetCustomErrorString();
		virtual bool GetTraceInfo(CallStackInfo *trace);
		virtual void ResetTrace();
		virtual const char *GetLastNative(uint32_t *index);
	private:
		TracedCall *m_pStart;
		TracedCall *m_pIterator;
		const char *m_pMsg;
		int m_Error;
		uint32_t m_Native;
	};


	class SourcePawnEngine : public ISourcePawnEngine
	{
	public:
		SourcePawnEngine();
		~SourcePawnEngine();
	public: //ISourcePawnEngine
		sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err);
		sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err);
		int FreeFromMemory(sp_plugin_t *plugin);
		IPluginContext *CreateBaseContext(sp_context_t *ctx);
		void FreeBaseContext(IPluginContext *ctx);
		void *BaseAlloc(size_t size);
		void BaseFree(void *memory);
		void *ExecAlloc(size_t size);
		void ExecFree(void *address);
		IDebugListener *SetDebugListener(IDebugListener *pListener);
		unsigned int GetContextCallCount();
	public: //Debugger Stuff
		/**
		 * @brief Pushes a context onto the top of the call tracer.
		 * 
		 * @param ctx		Plugin context.
		 */
		void PushTracer(sp_context_t *ctx);

		/**
		 * @brief Pops a plugin off the call tracer.
		 */
		void PopTracer(int error, const char *msg);

		/**
		 * @brief Runs tracer from a debug break.
		 */
		void RunTracer(sp_context_t *ctx, uint32_t frame, uint32_t codeip);
	private:
		TracedCall *MakeTracedCall(bool new_chain);
		void FreeTracedCall(TracedCall *pCall);
	private:
		IDebugListener *m_pDebugHook;
		TracedCall *m_FreedCalls;
		TracedCall *m_CallStack;
		unsigned int m_CurChain;
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
