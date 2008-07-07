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
	public:
		IProfiler *GetProfiler();
	private:
		IProfiler *m_Profiler;
	};
}

extern SourcePawn::SourcePawnEngine2 g_engine2;

#endif //_INCLUDE_SOURCEPAWN_ENGINE_2_H_
