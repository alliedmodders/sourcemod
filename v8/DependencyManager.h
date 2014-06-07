#pragma once

#include <ISourceMod.h>
#include <ILibrarySys.h>
#include <v8.h>
#include <string>
#include <map>
#include "ScriptLoader.h"

namespace SMV8
{
	using namespace std;
	using namespace v8;
	using namespace SourceMod;

	class DependencyManager
	{
	public:
		DependencyManager(Isolate *isolate, ISourceMod *sm, ILibrarySys *libsys, ScriptLoader *scriptLoader);
		virtual ~DependencyManager();
		void LoadDependencies(const string& package_path);
		string ResolvePath(const string& source_pkg, const string& require_path);
		void Depend(const string& depender, const string& pkg, const string& restriction);
		void ResetAliases(const string& depender);
		static void ext_readPakfile(const FunctionCallbackInfo<Value>& info);
		static void ext_findLocalVersions(const FunctionCallbackInfo<Value>& info);
		static const std::string packages_root;
	private:
		Handle<ObjectTemplate> BuildGlobalObjectTemplate();
		Isolate *isolate;
		ISourceMod *sm;
		ILibrarySys *libsys;
		ScriptLoader *scriptLoader;
		Persistent<Context> depman_context;
		Persistent<Object> jsDepMan;
	};
}