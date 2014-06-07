#include "SPAPIEmulation.h"
#include <IShareSys.h>
#include <string>
//#include <sp_vm_function.h>
#include "Marshaller.h"
#include <sstream>

namespace SMV8
{
	namespace SPEmulation
	{
		using namespace SourceMod;
		using namespace v8;

		struct sm_plugininfo_s_t
		{
			std::string name;
			std::string description;
			std::string author;
			std::string version;
			std::string url;
		};

		struct sm_plugininfo_c_t
		{
			cell_t name;
			cell_t description;
			cell_t author;
			cell_t version;
			cell_t url;
		};

		struct _ext
		{
			cell_t name;
			cell_t file;
			cell_t autoload;
			cell_t required;
		} *ext;

		PluginRuntime::PluginRuntime(Isolate* isolate, Manager *manager, Require::RequireManager *reqMan, ScriptLoader *script_loader, SMV8Script plugin_script)
			: pauseState(false), isolate(isolate), ctx(this), plugin_script(plugin_script), reqMan(reqMan),
			  script_loader(script_loader), manager(manager), current_script(&this->plugin_script)
		{
			HandleScope handle_scope(isolate);

			// Create a new context for this plugin, store it in a persistent handle.
			Handle<Context> ourContext = Context::New(isolate, NULL, GenerateGlobalObjectTemplate());
			v8Context.Reset(isolate, ourContext);

			Context::Scope context_scope(ourContext);

			SMV8Script intrinsics = script_loader->AutoLoadScript("v8/support/intrinsics");
			Script::Compile(String::New(intrinsics.GetCode().c_str()), String::New(intrinsics.GetPath().c_str()))->Run();

			TryCatch trycatch;
			Handle<Script> script = Script::Compile(String::New(plugin_script.GetCode().c_str()), String::New(plugin_script.GetPath().c_str()));
			Handle<Value> res = script->Run();

			if(res.IsEmpty())
			{
				Handle<Value> exception = trycatch.Exception();
				Handle<Value> stack_trace = trycatch.StackTrace();
				String::AsciiValue exceptionStr(exception);

				std::string err = *exceptionStr;
				if(!stack_trace.IsEmpty())
				{
					err += "\n";
					err += *String::AsciiValue(stack_trace.As<String>());
				}

				throw runtime_error(err);
			}

			ExtractPluginInfo();
			ExtractForwards();
			GenerateMaxClients();
		}

		PluginRuntime::~PluginRuntime()
		{
			for(PublicData* &pd: publics)
			{
				pd->func.Dispose();
				delete pd;
			}
			publics.clear();
			for(NativeData* &nd: natives)
			{
				delete nd;
			}
			natives.clear();
			v8Context.Dispose();
		}

		/**
		 * Generate the global object
		 */
		Handle<ObjectTemplate> PluginRuntime::GenerateGlobalObjectTemplate()
		{
			HandleScope handle_scope(isolate);
			Handle<ObjectTemplate> global(ObjectTemplate::New());
			global->Set("natives",GenerateNativesObjectTemplate());
			global->Set("plugin",GeneratePluginObjectTemplate());
			global->Set("forwards",ObjectTemplate::New());
			global->Set("require", FunctionTemplate::New(&Require, External::New(this)));
			global->Set("requireExt", FunctionTemplate::New(&RequireExt, External::New(this)));
			return handle_scope.Close(global);
		}

		void PluginRuntime::GetMaxClients(const FunctionCallbackInfo<Value>& info)
		{
			PluginRuntime* self = (PluginRuntime*)info.Data().As<External>()->Value();
			info.GetReturnValue().Set(Integer::New(*self->maxClients));
		}

		/**
		 * Generate the natives object
		 */
		Handle<ObjectTemplate> PluginRuntime::GenerateNativesObjectTemplate()
		{
			HandleScope handle_scope(isolate);
			Handle<ObjectTemplate> natives(ObjectTemplate::New());
			Handle<External> self = External::New(this);
			natives->Set("declare", FunctionTemplate::New(DeclareNative,self));
			natives->Set("GetMaxClients", FunctionTemplate::New(&GetMaxClients, External::New(this)));
			return handle_scope.Close(natives);
		}

		Handle<ObjectTemplate> PluginRuntime::GeneratePluginObjectTemplate()
		{
			HandleScope handle_scope(isolate);
			Handle<ObjectTemplate> plugin = ObjectTemplate::New();
			plugin->Set("info", GeneratePluginInfoObjectTemplate());
			return handle_scope.Close(plugin);
		}

		Handle<ObjectTemplate> PluginRuntime::GeneratePluginInfoObjectTemplate()
		{
			HandleScope handle_scope(isolate);
			Handle<ObjectTemplate> info = ObjectTemplate::New();
			info->Set("name", String::New("?????"));
			info->Set("description", String::New("God knows what this does..."));
			info->Set("author", String::New("Shitty programmer who can't write plugin info"));
			info->Set("version", String::New("v0.0.0.0.0.0.0.1aa"));
			info->Set("url", String::New("http://fillinyourplugininfo.dude"));
			return handle_scope.Close(info);
		}

		void PluginRuntime::ExtractForwards()
		{
			HandleScope handle_scope(isolate);

			Handle<Context> context(Handle<Context>::New(isolate,v8Context));
			Handle<Object> global(context->Global());
			Handle<Object> forwards(global->Get(String::New("forwards")).As<Object>());
			Handle<Array> keys = forwards->GetOwnPropertyNames();

			for(unsigned int i = 0; i < keys->Length(); i++)
			{
				Handle<String> key = keys->Get(i).As<String>();
				PublicData* pd = new PublicData;
				pd->name = *String::AsciiValue(key);
				pd->func.Reset(isolate,forwards->Get(key).As<Function>());
				pd->pfunc = new PluginFunction(*this, i);
				pd->state.funcid = i;
				pd->state.name = pd->name.c_str();
				pd->runtime = this;
				publics.push_back(pd);
			}
		}
		
		static cell_t InvalidV8Native(IPluginContext *pCtx, const cell_t *params)
		{
			return pCtx->ThrowNativeErrorEx(SP_ERROR_INVALID_NATIVE, "Invalid native");
		}

		void PluginRuntime::DeclareNative(const FunctionCallbackInfo<Value>& info)
		{
			PluginRuntime* self = (PluginRuntime*)info.Data().As<External>()->Value();
			HandleScope(self->isolate);

			if(info.Length() < 1)
				ThrowException(String::New("Invalid argument count"));

			// Check if this native is already loaded
			std::string nativeName = *String::AsciiValue(info[0].As<String>());
			uint32_t existingIdx;
			if(self->FindNativeByName(nativeName.c_str(), &existingIdx) == SP_ERROR_NONE)
				return;

			// Add the native to the database
			NativeData* nd = new NativeData;
			nd->runtime = self;
			nd->name = nativeName;
			nd->state.flags = 0;
			nd->state.pfn = InvalidV8Native;
			nd->state.status = SP_NATIVE_UNBOUND;
			nd->state.name = nd->name.c_str();
			nd->state.user = reinterpret_cast<void *>(self->natives.size());
			nd->resultType = info.Length() >= 2 ? (CellType)info[1].As<Integer>()->Value() : INT;
			self->natives.push_back(nd);

			self->RegisterNativeInNativesObject(*nd);
		}

		void PluginRuntime::RegisterNativeInNativesObject(NativeData& native)
		{
			HandleScope handle_scope(isolate);
			Handle<Context> context(Handle<Context>::New(isolate,v8Context));
			Handle<Object> global(context->Global());
			Handle<Object> oNatives = global->Get(String::New("natives")).As<Object>();
			Handle<External> ndata = External::New(&native);
			Handle<Function> func = FunctionTemplate::New(&NativeRouter,ndata)->GetFunction();
			oNatives->Set(String::New(native.name.c_str()), func);
		}

		void PluginRuntime::NativeRouter(const FunctionCallbackInfo<Value>& info)
		{
			NativeData* nd = (NativeData*)info.Data().As<External>()->Value();
			try
			{
	/*			if(info.Length() < nd->params.size())
				{
					ThrowException(String::New("Not enough parameters for native."));
				}
	*/
				V8ToSPMarshaller marshaller(*nd->runtime->isolate, *nd);
				info.GetReturnValue().Set(marshaller.HandleNativeCall(info));
			}
			catch(runtime_error& e)
			{
				info.GetReturnValue().Set(ThrowException(String::New(("Error when calling native function '" + nd->name + "': " + e.what()).c_str())));
			}
			catch(logic_error& e)
			{
				info.GetReturnValue().Set(ThrowException(String::New(("Error when calling native function '" + nd->name + "': " + e.what()).c_str())));
			}
		}

/*		void PluginRuntime::InsertNativeParams(NativeData& nd, Handle<Array> signature)
		{
			HandleScope(isolate);

			for(unsigned int i = 0; i < signature->Length(); i++)
			{
				Handle<Object> paramInfo = signature->Get(i).As<Object>();
				nd.params.push_back(CreateNativeParamInfo(paramInfo));
			}
		}

		NativeParamInfo PluginRuntime::CreateNativeParamInfo(Handle<Object> paramInfo)
		{
			Handle<String> name = paramInfo->Get(String::New("name")).As<String>();
			Handle<Integer> type = paramInfo->Get(String::New("type")).As<Integer>();

			String::AsciiValue nameAscii(name);

			return NativeParamInfo(*nameAscii, (CellType)type->Value());
		}
*/

		void PluginRuntime::Require(const FunctionCallbackInfo<Value>& info)
		{
			PluginRuntime* self = (PluginRuntime*)info.Data().As<External>()->Value();

			// We need to tell make sure that inside the  script we're requiring now, requires are made from that
			// script's perspective
			SMV8Script *curscript = self->current_script;
			SMV8Script s = self->script_loader->AutoLoadScript(self->reqMan->Require(*curscript, *String::AsciiValue(info[0].As<String>())));
			self->current_script = &s;

			HandleScope handle_scope(info.GetIsolate());
			Handle<Object> tmp = Object::New();

			// TODO: Security risk (or more like security suicide), try to execute in different context later
			string adjusted_code = "(function(){ var tmp = {}; (function(){ " + s.GetCode() + " }).call(tmp); return tmp; }).call(this)";

			Handle<Value> req = Script::Compile(String::New(adjusted_code.c_str()), String::New(s.GetPath().c_str()))->Run();

			self->current_script = curscript;

			info.GetReturnValue().Set(req);
		}

		Handle<Value> PluginRuntime::CallV8Function(funcid_t funcid, int argc, Handle<Value> argv[])
		{
			HandleScope handle_scope(isolate);

			PublicData* data(publics[funcid]);
			Handle<Function> func = Handle<Function>::New(isolate, data->func);

			return handle_scope.Close(func->Call(func, argc, argv));
		}

		/**
		 * Finds a suitable funcid for a volatile public
		 * Funcid's must remain constant for functions, so old entries are made NULL,
		 * but the entry is not removed from the public table.
		 * 
		 * This function will reuse nulled entries to prevent overflow.
		 */
		funcid_t PluginRuntime::AllocateVolatilePublic(PublicData *pd)
		{
			// Try to reuse funcid's
			for(int i = 0; i < publics.size(); i++)
			{
				if(publics[i] == NULL)
				{
					publics[i] = pd;
					return i;
				}
			}

			// Didn't find a reusable funcid.
			publics.push_back(pd);
			return publics.size() - 1;
		}

		/**
		 * Makes a "volatile" (weak reference) public for functions passed as parameter.
		 * This public is delisted as soon as it goes out of scope.
		 */
		funcid_t PluginRuntime::MakeVolatilePublic(Handle<Function> func)
		{
			PublicData *pd = new PublicData;
			funcid_t funcId = AllocateVolatilePublic(pd);

			ostringstream oss;
			oss << "____auto____public_" << funcId;

			pd->func.Reset(isolate,func);
			pd->name = oss.str();
			pd->pfunc = new PluginFunction(*this, funcId);
			pd->state.funcid = funcId;
			pd->state.name = pd->name.c_str();
			pd->runtime = this;

			pd->func.MakeWeak(isolate, pd, &VolatilePublicDisposer); 

			return funcId;
		}

		/**
		 * Callback which is called when a volatile public goes out of scope
		 */
		void PluginRuntime::VolatilePublicDisposer(Isolate* isolate, Persistent<Function> *func, PublicData* pd)
		{
			funcid_t funcId = pd->state.funcid;
			func->Dispose();
			pd->runtime->publics[funcId] = NULL;
			delete pd;
		}

		Isolate* PluginRuntime::GetIsolate()
		{
			return isolate;
		}

		void PluginRuntime::RequireExt(const FunctionCallbackInfo<Value>& info)
		{
			HandleScope handle_scope(info.GetIsolate());
			PluginRuntime* self = (PluginRuntime*)info.Data().As<External>()->Value();
			cell_t local_addr;
			_ext *extData;

			if(info.Length() < 2)
			{
				info.GetReturnValue().Set(ThrowException(String::New("Too few arguments")));
				return;
			}

			string name = *String::AsciiValue(info[0]->ToString());
			string file = *String::AsciiValue(info[1]->ToString());
			bool autoload = info.Length() >= 3 ? info[2]->ToBoolean()->Value() : true;
			bool required = info.Length() >= 4 ? info[3]->ToBoolean()->Value() : true;

			for(string &loadedExt: self->loadedExts)
			{
				if(loadedExt == file)
					return;
			}

			self->ctx.HeapAlloc(sizeof(_ext) / sizeof(cell_t), &local_addr, (cell_t **)&extData);
			self->LoadEmulatedString(name, extData->name);
			self->LoadEmulatedString(file, extData->file);
			extData->autoload = autoload;
			extData->required = required;

			ostringstream oss;
			oss << "__ext_" << self->pubvars.size();

			PubvarData pd;
			pd.local_addr = local_addr;
			pd.pubvar.offs = (cell_t *)extData;

			char* varname;
			self->ctx.HeapAlloc(4,&local_addr,(cell_t**)&varname);
			self->ctx.StringToLocal(local_addr, 16, oss.str().c_str());

			pd.pubvar.name = varname;

			self->pubvars.push_back(pd);
			self->loadedExts.push_back(file);
		}

		IPluginDebugInfo *PluginRuntime::GetDebugInfo()
		{
			return NULL;
		}

		int PluginRuntime::FindNativeByName(const char *name, uint32_t *index)
		{
			for(uint32_t i = 0; i < natives.size(); i++)
			{
				if(natives[i]->name == name)
				{
					*index = i;
					return SP_ERROR_NONE;
				}
			}

			return SP_ERROR_NOT_FOUND;
		}

		int PluginRuntime::GetNativeByIndex(uint32_t index, sp_native_t **native)
		{
			if(index >= natives.size())
				return SP_ERROR_INDEX;

			*native = &natives[index]->state;

			return SP_ERROR_NONE;
		}

		uint32_t PluginRuntime::GetNativesNum()
		{
			return natives.size();
		}

		// TODO: Implement forwards
		int PluginRuntime::FindPublicByName(const char *name, uint32_t *index)
		{
			for(uint32_t i = 0; i < publics.size(); i++)
			{
				if(publics[i]->name == name)
				{
					*index = i;
					return SP_ERROR_NONE;
				}
			}

			return SP_ERROR_NOT_FOUND;
		}

		int PluginRuntime::GetPublicByIndex(uint32_t index, sp_public_t **publicptr)
		{
			if(index >= publics.size())
				return SP_ERROR_INDEX;

			*publicptr = &publics[index]->state;

			return SP_ERROR_NONE;
		}

		uint32_t PluginRuntime::GetPublicsNum()
		{
			return publics.size();
		}

		/*
		 * Fake pubvars to publish dependencies
		 */
		void PluginRuntime::ExtractPluginInfo()
		{
			HandleScope handle_scope(isolate);
			sm_plugininfo_s_t realInfo;
			sm_plugininfo_c_t emulatedInfo;

			Handle<Context> context(Handle<Context>::New(isolate,v8Context));
			Handle<Object> global(context->Global());
			Handle<Object> plugin(global->Get(String::New("plugin")).As<Object>());
			Handle<Object> info(plugin->Get(String::New("info")).As<Object>());

			realInfo.name = *String::AsciiValue(info->Get(String::New("name")).As<String>());
			realInfo.description = *String::AsciiValue(info->Get(String::New("description")).As<String>());
			realInfo.author = *String::AsciiValue(info->Get(String::New("author")).As<String>());
			realInfo.version = *String::AsciiValue(info->Get(String::New("version")).As<String>());
			realInfo.url = *String::AsciiValue(info->Get(String::New("url")).As<String>());

			LoadEmulatedString(realInfo.name, emulatedInfo.name);
			LoadEmulatedString(realInfo.description, emulatedInfo.description);
			LoadEmulatedString(realInfo.author, emulatedInfo.author);
			LoadEmulatedString(realInfo.version, emulatedInfo.version);
			LoadEmulatedString(realInfo.url, emulatedInfo.url);

			cell_t local_addr;
			cell_t *phys_addr;
			ctx.HeapAlloc(sizeof(sm_plugininfo_c_t) / sizeof(cell_t), &local_addr, &phys_addr);
			memcpy(phys_addr, &emulatedInfo, sizeof(sm_plugininfo_c_t));

			PubvarData pd;
			pd.local_addr = local_addr;
			pd.pubvar.name = "myinfo";
			pd.pubvar.offs = phys_addr;

			pubvars.push_back(pd);
		}

		void PluginRuntime::LoadEmulatedString(const std::string& realstr, cell_t& local_addr_target)
		{
			cell_t local_addr;
			cell_t *phys_addr;
			size_t bytes_required = realstr.size() + 1;

			/* Calculate cells required for the string */
			size_t cells_required = (bytes_required + sizeof(cell_t) - 1) / sizeof(cell_t);
				
			ctx.HeapAlloc(cells_required, &local_addr, &phys_addr);
			ctx.StringToLocal(local_addr, bytes_required, realstr.c_str());
			local_addr_target = local_addr;
		}

		void PluginRuntime::GenerateMaxClients()
		{
			cell_t local_addr;
			ctx.HeapAlloc(1, &local_addr, &maxClients);

			PubvarData pd;
			pd.local_addr = local_addr;
			pd.pubvar.name = "MaxClients";
			pd.pubvar.offs = maxClients;

			pubvars.push_back(pd);
		}

		int PluginRuntime::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
		{
			if(index >= pubvars.size())
				return SP_ERROR_INDEX;

			*pubvar = &pubvars[index].pubvar;

			return SP_ERROR_NONE;
		}

		int PluginRuntime::FindPubvarByName(const char *name, uint32_t *index)
		{
			for(uint32_t i = 0; i < pubvars.size(); i++)
			{
				if(!strcmp(pubvars[i].pubvar.name,name))
				{
					*index = i;
					return SP_ERROR_NONE;
				}
			}

			return SP_ERROR_NOT_FOUND;
		}

		int PluginRuntime::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
		{
			if(index >= pubvars.size())
				return SP_ERROR_INDEX;

			*local_addr = pubvars[index].local_addr;
			*phys_addr = pubvars[index].pubvar.offs;

			return SP_ERROR_NONE;
		}

		uint32_t PluginRuntime::GetPubVarsNum()
		{
			return pubvars.size();
		}

		IPluginFunction *PluginRuntime::GetFunctionByName(const char *public_name)
		{
			uint32_t idx;
			if(FindPublicByName(public_name, &idx) != SP_ERROR_NONE)
				return NULL;

			return GetFunctionById(idx);
		}

		IPluginFunction *PluginRuntime::GetFunctionById(funcid_t func_id)
		{
			return publics[func_id]->pfunc;
		}

		IPluginContext *PluginRuntime::GetDefaultContext()
		{
			return &ctx;
		}

		bool PluginRuntime::IsDebugging()
		{
			return true;
		}

		int PluginRuntime::ApplyCompilationOptions(ICompilation *co)
		{
			return false;
		}

		void PluginRuntime::SetPauseState(bool paused)
		{
			pauseState = paused;
		}

		bool PluginRuntime::IsPaused()
		{
			return pauseState;
		}

		size_t PluginRuntime::GetMemUsage()
		{
			return 0;
		}

		unsigned char *PluginRuntime::GetCodeHash()
		{
			return (unsigned char *)"34234663464";
		}

		unsigned char *PluginRuntime::GetDataHash()
		{
			return (unsigned char *)"453253543";
		}	
	}
}
