#pragma once
#include <IV8Manager.h>
#include "ScriptLoader.h"
#include "DependencyManager.h"
#include "Require/RequireManager.h"

namespace SMV8
{
	using namespace v8;
	using namespace SourceMod;

	class Manager : public IManager
	{
	public:
		Manager();
		virtual void Initialize(ISourceMod *sm, ILibrarySys *libsys, const char **err);
		virtual SourcePawn::IPluginRuntime *LoadPlugin(char* filename, const char **err);
		SourcePawn::IPluginRuntime *LoadPakPlugin(const string& package_name);
		void ReportError(SourcePawn::IPluginContext *ctx, funcid_t funcid, int err, const std::string& msg, ...);
		virtual void SetDebugListener(SourcePawn::IDebugListenerV8 *debug_listener);
		virtual ~Manager(void);
	private:
		Isolate *isolate;
		ScriptLoader *scriptLoader;
		ISourceMod *sm;
		DependencyManager *depMan;
		Require::RequireManager *reqMan;
		std::string last_error;
		SourcePawn::IDebugListenerV8 *debug_listener;
	};
}