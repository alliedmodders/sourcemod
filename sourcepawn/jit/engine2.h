// vim: set ts=4 sw=4 tw=99 noet:
#ifndef _INCLUDE_SOURCEPAWN_ENGINE_2_H_
#define _INCLUDE_SOURCEPAWN_ENGINE_2_H_

#include <sp_vm_api.h>

namespace SourcePawn
{
	/** 
	 * @brief Outlines the interface a Virtual Machine (JIT) must expose
	 */
	class SourcePawnEngine2 : public ISourcePawnEngine2
	{
	public:
		SourcePawnEngine2();
	public:
		unsigned int GetAPIVersion();
		const char *GetEngineName();
		const char *GetVersionString();
		IPluginRuntime *LoadPlugin(ICompilation *co, const char *file, int *err);
		SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData);
		void DestroyFakeNative(SPVM_NATIVE_FUNC func);
		IDebugListener *SetDebugListener(IDebugListener *listener);
		void SetProfiler(IProfiler *profiler);
		ICompilation *StartCompilation();
		const char *GetErrorString(int err);
		bool Initialize();
		void Shutdown();
		IPluginRuntime *CreateEmptyRuntime(const char *name, uint32_t memory);
		bool InstallWatchdogTimer(size_t timeout_ms);

		bool SetJitEnabled(bool enabled) {
			jit_enabled_ = enabled;
			return true;
	    }

		bool IsJitEnabled() {
			return jit_enabled_;
		}
	public:
		IProfiler *GetProfiler();
	private:
		IProfiler *m_Profiler;
		bool jit_enabled_;
	};
}

extern SourcePawn::SourcePawnEngine2 g_engine2;

#endif //_INCLUDE_SOURCEPAWN_ENGINE_2_H_
