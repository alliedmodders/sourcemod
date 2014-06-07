#include "ScriptLoader.h"
#include <sm_platform.h>
#include <fstream>
#include <sstream>
#include "DependencyManager.h"

namespace SMV8
{
	using namespace std;
	
	SMV8Script::SMV8Script(std::string code, std::string path)
		: code(code), path(path)
	{
	}

	SMV8Script::SMV8Script(const SMV8Script& other)
	{
		code = other.code;
		path = other.path;
	}

	SMV8Script::~SMV8Script()
	{
	}

	std::string SMV8Script::GetCode() const
	{
		return code;
	}

	std::string SMV8Script::GetPath() const
	{
		return path;
	}

	bool SMV8Script::IsInPackage() const
	{
		return GetPath().find(DependencyManager::packages_root) == 0;
	}

	std::string SMV8Script::GetPackage() const
	{
		if(!IsInPackage())
			throw logic_error("Calling GetPackage on non-packaged script.");

		string::size_type slashloc;
		string localdir = GetPath().substr(DependencyManager::packages_root.size());
		slashloc = localdir.find('/');
		return localdir.substr(0, slashloc);
	}

	std::string SMV8Script::GetVersionedPackage() const
	{
		if(!IsInPackage())
			throw logic_error("Calling GetVersionedPackage on non-packaged script.");

		string::size_type slashloc;
		string localdir = GetPath().substr(DependencyManager::packages_root.size());
		slashloc = localdir.find('/');
		slashloc = localdir.find('/',slashloc + 1);
		return localdir.substr(0, slashloc);
	}
	
	ScriptLoader::ScriptLoader(Isolate *isolate, ISourceMod *sm)
		: isolate(isolate), sm(sm)
	{
		HandleScope handle_scope(isolate);
		coffeeCompilerContext.Reset(isolate, Context::New(isolate));
		LoadCoffeeCompiler(sm);
	}

	ScriptLoader::~ScriptLoader()
	{
		coffeeCompilerContext.Dispose();
	}

	SMV8Script ScriptLoader::AutoLoadScript(const std::string& location) const
	{
		char fullpath[PLATFORM_MAX_PATH];
		sm->BuildPath(Path_SM, fullpath, sizeof(fullpath), location.c_str());
		string sfullpath(fullpath);

		ifstream ifs(sfullpath + ".coffee");
		ostringstream content;

		if(ifs.is_open())
		{
			content << ifs.rdbuf();
			return SMV8Script(CompileCoffee(content.str()), location + ".coffee");
		}

		ifs.open(sfullpath + ".js");
		if(ifs.is_open())
		{
			content << ifs.rdbuf();
			return SMV8Script(content.str(), location + ".js");
		}

		throw runtime_error("Script can't be loaded: " + sfullpath);
	}

	SMV8Script ScriptLoader::LoadScript(const std::string& location, bool forceCoffee) const
	{
		char fullpath[PLATFORM_MAX_PATH];
		sm->BuildPath(Path_SM, fullpath, sizeof(fullpath), location.c_str());
		string sfullpath(fullpath);

		ifstream ifs(fullpath);
		if(!ifs.is_open())
			throw runtime_error("Script can't be loaded" + sfullpath);

		ostringstream oss;
		oss << ifs.rdbuf();

		if(forceCoffee || sfullpath.find(".coffee", sfullpath.size() - 7) != string::npos)
			return SMV8Script(CompileCoffee(oss.str()), location);	

		return SMV8Script(oss.str(), location);
	}

	void ScriptLoader::LoadCoffeeCompiler(ISourceMod *sm)
	{
		HandleScope handle_scope(isolate);

		char fullpath[PLATFORM_MAX_PATH];
		sm->BuildPath(Path_SM, fullpath, sizeof(fullpath), "v8/support/coffee_compiler.js");

		ifstream ifs(fullpath);
		if(!ifs.is_open())
			throw runtime_error("CoffeeScript compiler not found.");

		ostringstream oss;
		oss << ifs.rdbuf();

		Handle<Context> context = Handle<Context>::New(isolate, coffeeCompilerContext);
		Context::Scope context_scope(context);

		Handle<Script> coffeeCompiler = Script::Compile(String::New(oss.str().c_str()));
		coffeeCompiler->Run();
	}

	std::string ScriptLoader::CompileCoffee(const std::string& coffee) const
	{
		HandleScope handle_scope(isolate);

		Handle<Context> context = Handle<Context>::New(isolate, coffeeCompilerContext);
		Context::Scope context_scope(context);

		Handle<Object> coffeescript = context->Global()->Get(String::New("CoffeeScript")).As<Object>();

		const int argc = 1;
		Handle<Value> argv[argc] = { String::New(coffee.c_str()) };

		TryCatch trycatch;
		Handle<Value> result = coffeescript->Get(String::New("compile")).As<Function>()->Call(coffeescript, argc, argv);
		if(result.IsEmpty())
		{
			Handle<Value> exception = trycatch.Exception();
			String::AsciiValue exceptionStr(exception);

			std::string err = *exceptionStr;

			throw runtime_error("Can't compile CoffeeScript: " + err);
		}

		String::Utf8Value jsutf8(result.As<String>());
		return *jsutf8;
	}

	// TODO: also check extensions
	bool ScriptLoader::CanLoad(const std::string& location) const
	{
		char fullpath[PLATFORM_MAX_PATH];
		sm->BuildPath(Path_SM, fullpath, sizeof(fullpath), location.c_str());
		string sfullpath(fullpath);

		ifstream ifs(fullpath);
		return ifs.is_open();
	}

	// TODO: also check extensions
	bool ScriptLoader::CanAutoLoad(const std::string& location) const
	{
		char fullpath[PLATFORM_MAX_PATH];
		sm->BuildPath(Path_SM, fullpath, sizeof(fullpath), location.c_str());
		string sfullpath(fullpath);

		ifstream ifs(sfullpath + ".coffee");
		ostringstream content;

		if(ifs.is_open())
			return true;

		ifs.open(sfullpath + ".js");
		if(ifs.is_open())
			return true;

		return false;
	}
}