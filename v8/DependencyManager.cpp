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
		Local<Context> context = Context::New(isolate, NULL, BuildGlobalObjectTemplate());
		depman_context.Reset(isolate, context);
		Context::Scope context_scope(context);
		Script::Compile(String::NewFromUtf8(isolate, pkgman.GetCode().c_str()))->Run().As<Object>();
		Local<Object> depMan = context->Global()->Get(String::NewFromUtf8(isolate, "dependencyManager")).As<Object>();
		bool depManEmpty = depMan.IsEmpty();
		bool depManObj = depMan->IsObject();
		jsDepMan.Reset(isolate, depMan);
	}

	DependencyManager::~DependencyManager()
	{
		jsDepMan.Reset();
		depman_context.Reset();
	}

	void DependencyManager::LoadDependencies(const string& package_path)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Local<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Local<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 1;
		Local<Value> argv[argc] = {String::NewFromUtf8(isolate, package_path.c_str())};
		depMan->Get(String::NewFromUtf8(isolate, "loadDependencies")).As<Function>()->Call(depMan, argc, argv);
	}

	// TODO: Throw error if not found
	string DependencyManager::ResolvePath(const string& source_pkg, const string& require_path)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Local<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Local<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 2;
		Local<Value> argv[argc] = {String::NewFromUtf8(isolate, source_pkg.c_str()), String::NewFromUtf8(isolate, require_path.c_str())};
		Local<String> result = depMan->Get(String::NewFromUtf8(isolate, "resolvePath")).As<Function>()->Call(depMan, argc, argv).As<String>();

		return *String::Utf8Value(result);
	}

	void DependencyManager::Depend(const string& depender, const string& pkg, const string& restriction)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Local<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Local<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 3;
		Local<Value> argv[argc] = {String::NewFromUtf8(isolate, depender.c_str()), String::NewFromUtf8(isolate, pkg.c_str()), String::NewFromUtf8(isolate, restriction.c_str())};

		TryCatch trycatch;
		Local<Value> result = depMan->Get(String::NewFromUtf8(isolate, "depend")).As<Function>()->Call(depMan, argc, argv);

		if(result.IsEmpty())
		{
			Local<Value> err = trycatch.Exception();
			string exceptionStr = *String::Utf8Value(err);

			throw runtime_error(exceptionStr);
		}
	}

	void DependencyManager::ResetAliases(const string& depender)
	{
		HandleScope handle_scope(isolate);

		Local<Context> context = Local<Context>::New(isolate,depman_context);
		Context::Scope context_scope(context);

		Local<Object> depMan = Local<Object>::New(isolate, jsDepMan);

		const unsigned int argc = 1;
		Local<Value> argv[argc] = {String::NewFromUtf8(isolate, depender.c_str())};

		depMan->Get(String::NewFromUtf8(isolate, "resetAliases")).As<Function>()->Call(depMan, argc, argv);
	}

	Local<ObjectTemplate> DependencyManager::BuildGlobalObjectTemplate()
	{
		EscapableHandleScope handle_scope(isolate);
		Local<ObjectTemplate> externals = ObjectTemplate::New();
		externals->Set(String::NewFromUtf8(isolate, "readPakfile"), FunctionTemplate::New(isolate, &ext_readPakfile,External::New(isolate, this)));
		externals->Set(String::NewFromUtf8(isolate, "findLocalVersions"), FunctionTemplate::New(isolate, &ext_findLocalVersions,External::New(isolate, this)));
		Local<ObjectTemplate> gt = ObjectTemplate::New();
		gt->Set(String::NewFromUtf8(isolate, "externals"), externals);
		return handle_scope.Escape(gt);
	}

	void DependencyManager::ext_readPakfile(const FunctionCallbackInfo<Value>& info)
	{
		DependencyManager *self = (DependencyManager *)info.Data().As<External>()->Value();
		string package = *String::Utf8Value(info[0].As<String>());
		string pkgfile = self->packages_root + package + "/Pakfile";

		SMV8Script s = self->scriptLoader->LoadScript(pkgfile, true);
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), s.GetCode().c_str()));
	}

	void DependencyManager::ext_findLocalVersions(const FunctionCallbackInfo<Value>& info)
	{
		HandleScope handle_scope(info.GetIsolate());
		DependencyManager *self = (DependencyManager *)info.Data().As<External>()->Value();
		string package = *String::Utf8Value(info[0].As<String>());
		string packageDir = self->packages_root + package;
		char pkgCollectionPath[PLATFORM_MAX_PATH];

		self->sm->BuildPath(Path_SM, pkgCollectionPath, sizeof(pkgCollectionPath), packageDir.c_str());

		Local<Array> res = Array::New(info.GetIsolate());

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
					res->Set(i++, String::NewFromUtf8(info.GetIsolate(), name));
			}

			pkgCollectionDir->NextEntry();
		}
		self->libsys->CloseDirectory(pkgCollectionDir);

		info.GetReturnValue().Set(res);
	}
}