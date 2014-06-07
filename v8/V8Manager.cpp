#include "V8Manager.h"

#include "SPAPIEmulation.h"


namespace SMV8
{
	using namespace std;
	using namespace v8;
	using namespace SourceMod;

	Manager::Manager()
	{
	}

	void Manager::Initialize(ISourceMod *sm, ILibrarySys *libsys, const char **err)
	{
		try
		{
			*err = NULL;
			isolate = Isolate::GetCurrent();
			HandleScope handle_scope(isolate);
			scriptLoader = new ScriptLoader(isolate, sm);
			depMan = new DependencyManager(isolate, sm, libsys, scriptLoader);
			reqMan = new Require::RequireManager(sm, libsys, depMan, scriptLoader);
			this->sm = sm;
		}
		catch(runtime_error& e)
		{
			last_error = string("Runtime error while loading V8 engine: ") + e.what();
			*err = last_error.c_str();
		}
		catch(logic_error& e)
		{
			last_error = string("Logic error while loading V8 engine: ") + e.what();
			*err = last_error.c_str();
		}
	}

	void Manager::SetDebugListener(SourcePawn::IDebugListenerV8 *debug_listener)
	{
		this->debug_listener = debug_listener;
	}

	SourcePawn::IPluginRuntime *Manager::LoadPlugin(char* filename, const char **err)
	{
		try
		{
			*err = NULL;
			string sfilename(filename);

			auto pakPluginLoc = sfilename.find(".pakplugin", sfilename.size() - 10);

			if(pakPluginLoc != string::npos)
				return LoadPakPlugin(sfilename.substr(0, pakPluginLoc));	

			string fake_package_id = "__nopak__" + sfilename;
			depMan->ResetAliases(fake_package_id);
			depMan->Depend(fake_package_id, "sourcemod", ">= 0");
			return new SPEmulation::PluginRuntime(isolate, this, reqMan, scriptLoader, scriptLoader->LoadScript("plugins/" + sfilename));
		}
		catch(runtime_error& e)
		{
			last_error = string("Runtime error while loading V8 plugin: ") + e.what();
			*err = last_error.c_str();
			return NULL;
		}
		catch(logic_error& e)
		{
			last_error = string("Logic error while loading V8 plugin: ") + e.what();
			*err = last_error.c_str();
			return NULL;
		}
	}

	SourcePawn::IPluginRuntime *Manager::LoadPakPlugin(const string& package_name)
	{
		try
		{
			string fake_package_id = "__pakplugin__" + package_name;
			depMan->ResetAliases(fake_package_id);
			depMan->Depend(fake_package_id, package_name, ">= 0");
			string script_path = DependencyManager::packages_root + depMan->ResolvePath(fake_package_id,package_name) + "/main";

			return new SPEmulation::PluginRuntime(isolate, this, reqMan, scriptLoader, scriptLoader->AutoLoadScript(script_path));
		}
		catch(runtime_error& err)
		{
			throw runtime_error("Can't load package-based plugin: " + string(err.what()));
		}
	}
	
	void Manager::ReportError(SourcePawn::IPluginContext *ctx, funcid_t funcid, int err, const std::string& msg, ...)
	{
		va_list va;
		va_start(va, msg);
		debug_listener->GenerateErrorVA(ctx, (funcid << 1) | 1, err, msg.c_str(), va);
		va_end(va);
	}

	Manager::~Manager(void)
	{
		delete reqMan;
		delete depMan;
		delete scriptLoader;
	}
}