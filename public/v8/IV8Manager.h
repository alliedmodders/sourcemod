#ifndef _INCLUDE_PUBLIC_V8MANAGER_H_
#define _INCLUDE_PUBLIC_V8MANAGER_H_

#include <v8.h>

#include <ISourceMod.h>
#include <sp_vm_api.h>
#undef min
#undef max
#include <string>
#include <ILibrarySys.h>

namespace SMV8
{
	using namespace v8;
	using namespace SourceMod;

	class IManager
	{
	public:
		virtual void Initialize(ISourceMod *sm, ILibrarySys *libsys, const char **err) = 0;
		virtual SourcePawn::IPluginRuntime *LoadPlugin(char* filename, const char **err) = 0;
		virtual void SetDebugListener(SourcePawn::IDebugListenerV8 *debug_listener) = 0;
	};

}

typedef SMV8::IManager* (*GET_V8)();

#endif // !defined _INCLUDE_PUBLIC_V8MANAGER_H_
