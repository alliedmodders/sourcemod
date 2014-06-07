#include "DependencyManager.h"
#include <sm_platform.h>
#include <fstream>
#include <sstream>
#include <ILibrarySys.h>

namespace SMV8
{
	using namespace std;
	using namespace SourceMod;
	using namespace v8;

	const std::string DependencyManager::packages_root("v8/packages/");

	DependencyManager::DependencyManager(Isolate *isolate, ISourceMod *sm, ILibrarySys *libsys, ScriptLoader* scriptLoader)
		: isolate(isolate), sm(sm), libsys(libsys), scriptLoader(scriptLoader)
	{
		SMV8Script pkgman = scriptLoader->LoadScript("v8/support/pkgman.coffee");

		HandleScope handle_scope(isolate);
		Handle<Context> context = Context::New(isolate, NULL, BuildGlobalObjectTemplate());
		depman_context.Reset(isolate, context);
		Context::Scope context_scope(context);
		Script::Compile(String::New(pkgman.GetCode().c_str()))->Run().As<Object>();
		Handle<Object> depMan = context->Global()->Get(String::New("dependencyManager")).As<Object>();
		bool depManEmpty = depMan.IsEmpty();
		bool depManObj = depMan->IsObject();
		jsDepMan.Reset(isolate, depMan);
	}

	DependencyManager::~DependencyManager()
	{
		jsDepMan.Dispose();
		depman_context.Dispose();
	}

	void DependencyManager::LoadDependencies(const string& package_path)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Handle<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Handle<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 1;
		Handle<Value> argv[argc] = {String::New(package_path.c_str())};
		depMan->Get(String::New("loadDependencies")).As<Function>()->Call(depMan, argc, argv);
	}

	// TODO: Throw error if not found
	string DependencyManager::ResolvePath(const string& source_pkg, const string& require_path)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Handle<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Handle<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 2;
		Handle<Value> argv[argc] = {String::New(source_pkg.c_str()), String::New(require_path.c_str())};
		Handle<String> result = depMan->Get(String::New("resolvePath")).As<Function>()->Call(depMan, argc, argv).As<String>();

		return *String::AsciiValue(result);
	}

	void DependencyManager::Depend(const string& depender, const string& pkg, const string& restriction)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Handle<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Handle<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 3;
		Handle<Value> argv[argc] = {String::New(depender.c_str()), String::New(pkg.c_str()), String::New(restriction.c_str())};

		TryCatch trycatch;
		Handle<Value> result = depMan->Get(String::New("depend")).As<Function>()->Call(depMan, argc, argv);

		if(result.IsEmpty())
		{
			Handle<Value> err = trycatch.Exception();
			string exceptionStr = *String::AsciiValue(err);

			throw runtime_error(exceptionStr);
		}
	}

	void DependencyManager::ResetAliases(const string& depender)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Handle<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Handle<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 1;
		Handle<Value> argv[argc] = {String::New(depender.c_str())};

		depMan->Get(String::New("resetAliases")).As<Function>()->Call(depMan, argc, argv);
	}

	Handle<ObjectTemplate> DependencyManager::BuildGlobalObjectTemplate()
	{
		HandleScope handle_scope(isolate);
		Handle<ObjectTemplate> externals = ObjectTemplate::New();
		externals->Set("readPakfile",FunctionTemplate::New(&ext_readPakfile,External::New(this)));
		externals->Set("findLocalVersions",FunctionTemplate::New(&ext_findLocalVersions,External::New(this)));
		Handle<ObjectTemplate> gt = ObjectTemplate::New();
		gt->Set("externals", externals);
		return handle_scope.Close(gt);
	}

	void DependencyManager::ext_readPakfile(const FunctionCallbackInfo<Value>& info)
	{
		DependencyManager *self = (DependencyManager *)info.Data().As<External>()->Value();
		string package = *String::AsciiValue(info[0].As<String>());
		string pkgfile = self->packages_root + package + "/Pakfile";

		SMV8Script s = self->scriptLoader->LoadScript(pkgfile, true);
		info.GetReturnValue().Set(String::New(s.GetCode().c_str()));
	}

	void DependencyManager::ext_findLocalVersions(const FunctionCallbackInfo<Value>& info)
	{
		HandleScope handle_scope(info.GetIsolate());
		DependencyManager *self = (DependencyManager *)info.Data().As<External>()->Value();
		string package = *String::AsciiValue(info[0].As<String>());
		string packageDir = self->packages_root + package;
		char pkgCollectionPath[PLATFORM_MAX_PATH];

		self->sm->BuildPath(Path_SM, pkgCollectionPath, sizeof(pkgCollectionPath), packageDir.c_str());

		Handle<Array> res = Array::New();

		if(!self->libsys->PathExists(pkgCollectionPath) || !self->libsys->IsPathDirectory(pkgCollectionPath))
		{
			info.GetReturnValue().Set(res);
			return;
		}

		IDirectory *pkgCollectionDir = self->libsys->OpenDirectory(pkgCollectionPath);
		unsigned int i = 0;
		while(pkgCollectionDir->MoreFiles())
		{
			if(pkgCollectionDir->IsEntryDirectory())
			{
				const char* name = pkgCollectionDir->GetEntryName();
				if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
					res->Set(i++, String::New(name));
			}

			pkgCollectionDir->NextEntry();
		}
		self->libsys->CloseDirectory(pkgCollectionDir);

		info.GetReturnValue().Set(res);
	}
}