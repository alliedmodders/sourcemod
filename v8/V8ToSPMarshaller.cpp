#include "Marshaller.h"
#include <math.h>
#include <sstream>
#include <stdexcept>

namespace SMV8
{
	namespace SPEmulation
	{
		using namespace std;

		V8ToSPMarshaller::V8ToSPMarshaller(Isolate& isolate, NativeData& native)
			: isolate(isolate), native(native), runtime(*native.runtime), ctx(*runtime.GetDefaultContext())
		{
		}

		V8ToSPMarshaller::~V8ToSPMarshaller()
		{
		}

		Local<Value> V8ToSPMarshaller::HandleNativeCall(const FunctionCallbackInfo<Value>& info)
		{
			EscapableHandleScope handle_scope(&isolate);

			cell_t params[SP_MAX_EXEC_PARAMS];

			params[0] = info.Length();

			for(int i = 0; i < std::min(info.Length(), SP_MAX_EXEC_PARAMS); i++)
			{
				try
				{
					PushParam(info[i], &params[i+1], false);
				}
				catch(runtime_error& e)
				{
					ostringstream oss;
					oss << "Error marshalling parameter " << i << ": " << e.what();
					throw runtime_error(oss.str());
				}
			}

			cell_t result = native.state.pfn(&ctx, params);

			CopyBackRefs();

			return handle_scope.Escape(GetResult(result));
		}

		void V8ToSPMarshaller::PushParam(Local<Value> val, cell_t* param_dst, bool forcefloat)
		{
			if(val->IsFunction())
				PushFunction(val.As<Function>(), param_dst);
			else if(val->IsObject())
				PushObject(val.As<Object>(), param_dst, forcefloat);
			else if(!forcefloat && val->IsInt32())
				PushInt(val.As<Integer>(), param_dst);
			else if(val->IsNumber())
				PushFloat(val.As<Number>(), param_dst);
			else if(val->IsBoolean())
				PushBool(val->ToBoolean(), param_dst);
			else if(val->IsArray() || val->IsString())
				PushByRef(WrapInDummyObject(val), param_dst, forcefloat);
			else 
				throw runtime_error("Unacceptable argument type");
		}

		Local<Object> V8ToSPMarshaller::WrapInDummyObject(Local<Value> val)
		{
			EscapableHandleScope handle_scope(&isolate);
			Local<Object> dummy = Object::New(&isolate);
			dummy->Set(String::NewFromUtf8(&isolate, "value"), val);
			return handle_scope.Escape(dummy);
		}

		void V8ToSPMarshaller::PushObject(Local<Object> val, cell_t* param_dst, bool forcefloat)
		{
			if(val->Has(String::NewFromUtf8(&isolate, "forcefloat")))
				PushParam(val->Get(String::NewFromUtf8(&isolate, "value")), param_dst, true);
			else
				PushByRef(val.As<Object>(), param_dst, forcefloat);
		}

		void V8ToSPMarshaller::PushByRef(Local<Object> val, cell_t* param_dst, bool forcefloat)
		{
			HandleScope handle_scope(&isolate);
			Local<Value> realVal = val->Get(String::NewFromUtf8(&isolate, "value"));

			if(realVal->IsString()) {
				PushString(realVal.As<String>(), val, param_dst);
				return;
			}

			if(realVal->IsArray()) {
				PushArray(realVal.As<Array>(), val, param_dst, forcefloat);
				return;
			}

			cell_t dst_local;
			cell_t* dst_phys;
			ctx.HeapAlloc(1, &dst_local, &dst_phys);

			ReferenceInfo *ri = new ReferenceInfo;
			ri->addr = dst_local;
			ri->refObj.Reset(&isolate, val);
			ri->forcefloat = forcefloat;
			refs.push(ri);
			
			PushParam(realVal,dst_phys,forcefloat);
			*param_dst = dst_local;
		}

		void V8ToSPMarshaller::PushInt(Local<Integer> val, cell_t* param_dst)
		{
			*param_dst = static_cast<cell_t>(val->Value());
		}

		void V8ToSPMarshaller::PushFloat(Local<Number> val, cell_t* param_dst)
		{
			float fNumb = static_cast<float>(val->Value());
			*param_dst = sp_ftoc(fNumb);
		}

		void V8ToSPMarshaller::PushBool(Local<Boolean> val, cell_t* param_dst)
		{
			*param_dst = static_cast<cell_t>(val->Value());
		}

		void V8ToSPMarshaller::PushArray(Local<Array> val, Local<Object> refObj, cell_t* param_dst, bool forcefloat)
		{
			size_t cells_required = val->Length();

			// If this ref is carrying a size parameter for us, we set that size instead.
			if(refObj->Has(String::NewFromUtf8(&isolate, "size")))
				cells_required = refObj->Get(String::NewFromUtf8(&isolate, "size")).As<Integer>()->Value();
			
			cell_t dst_local;
			cell_t* dst_phys;
			ctx.HeapAlloc(cells_required, &dst_local, &dst_phys);

			ReferenceInfo *ri = new ReferenceInfo;
			ri->addr = dst_local;
			ri->refObj.Reset(&isolate, refObj);
			ri->forcefloat = forcefloat;
			refs.push(ri);

			for(unsigned int i = 0; i < cells_required; i++)
			{
				PushParam(val->Get(i), dst_phys + i, forcefloat);
			}

			*param_dst = dst_local;
		}

		void V8ToSPMarshaller::PushString(Local<String> val, Local<Object> refObj, cell_t* param_dst)
		{
			string str = *String::Utf8Value(val);
			size_t bytes_required = str.size() + 1;

			// If this ref is carrying a size parameter for us, we set that size instead.
			if(refObj->Has(String::NewFromUtf8(&isolate, "size")))
				bytes_required = refObj->Get(String::NewFromUtf8(&isolate, "size")).As<Integer>()->Value();

			/* Calculate cells required for the string */
			size_t cells_required = (bytes_required + sizeof(cell_t) - 1) / sizeof(cell_t);
			
			cell_t dst_local;
			cell_t* dst_phys;
			ctx.HeapAlloc(cells_required, &dst_local, &dst_phys);

			ReferenceInfo *ri = new ReferenceInfo;
			ri->addr = dst_local;
			ri->refObj.Reset(&isolate, refObj);
			ri->forcefloat = false;
			refs.push(ri);

			ctx.StringToLocalUTF8(dst_local, bytes_required, str.c_str(), NULL);
			*param_dst = dst_local;
		}

		void V8ToSPMarshaller::PushFunction(Local<Function> val, cell_t* param_dst)
		{
			funcid_t funcId = runtime.EnsureVolatilePublic(val);
			*param_dst = funcId;
		}

		Local<Value> V8ToSPMarshaller::GetResult(cell_t result)
		{
			switch(native.resultType)
			{
			case INT:
				return Integer::New(&isolate, result);
			case FLOAT:
				return Number::New(&isolate, sp_ctof(result));
			case BOOL:
				return Boolean::New(&isolate, (bool)result);
			default:
				throw runtime_error("Illegal result type");
			}
		}

		void V8ToSPMarshaller::CopyBackRefs()
		{
			HandleScope handle_scope(&isolate);

			for(int i = refs.size() - 1; i >= 0; i--)
			{
				ReferenceInfo *ri = refs.top();
				Local<Object> refObj = Local<Object>::New(&isolate, ri->refObj);

				refObj->Set(String::NewFromUtf8(&isolate, "value"), PopRef(ri));

				ri->refObj.Reset();
				delete ri;
				refs.pop();
			}
		}

		Local<Value> V8ToSPMarshaller::PopRef(ReferenceInfo *ri)
		{
			EscapableHandleScope handle_scope(&isolate);
			Local<Object> refObj = Local<Object>::New(&isolate, ri->refObj);
			Local<Value> val = refObj->Get(String::NewFromUtf8(&isolate, "value"));

			if(!ri->forcefloat && val->IsInt32())
				return handle_scope.Escape(PopIntRef(ri));
			else if(val->IsNumber())
				return handle_scope.Escape(PopFloatRef(ri));
			else if(val->IsString())
				return handle_scope.Escape(PopString(ri));
			else if(val->IsArray())
				return handle_scope.Escape(PopArray(ri));
			else
				throw logic_error("V8ToSPMarshaller: Invalid argument type in copyback stack.");
		}

		Local<Integer> V8ToSPMarshaller::PopIntRef(ReferenceInfo *ri)
		{
			cell_t addr = ri->addr;
			cell_t* phys_addr;

			ctx.LocalToPhysAddr(addr, &phys_addr);

			cell_t val = *phys_addr;

			ctx.HeapPop(addr);

			return Integer::New(&isolate, val).As<Integer>();
		}

		Local<Number> V8ToSPMarshaller::PopFloatRef(ReferenceInfo *ri)
		{
			cell_t addr = ri->addr;
			cell_t* phys_addr;

			ctx.LocalToPhysAddr(addr, &phys_addr);

			float val = sp_ctof(*phys_addr);

			ctx.HeapPop(addr);

			return Number::New(&isolate, val);
		}

		Local<String> V8ToSPMarshaller::PopString(ReferenceInfo *ri)
		{
			EscapableHandleScope handle_scope(&isolate);
			cell_t addr = ri->addr;
			char* str;

			ctx.LocalToString(addr, &str);

			Local<String> result = String::NewFromUtf8(&isolate, str);

			ctx.HeapPop(addr);

			return handle_scope.Escape(result);
		}

		Local<Array> V8ToSPMarshaller::PopArray(ReferenceInfo *ri)
		{
			cell_t addr = ri->addr;
			cell_t* phys_addr;

			ctx.LocalToPhysAddr(addr, &phys_addr);

			EscapableHandleScope handle_scope(&isolate);
			Local<Object> refObj = Local<Object>::New(&isolate, ri->refObj);
			int size = refObj->Get(String::NewFromUtf8(&isolate, "size")).As<Integer>()->Value();

			Local<Array> arr = Array::New(&isolate);

			if(ri->forcefloat)
			{
				for(int i = 0; i < size; i++)
				{
					arr->Set(i, Number::New(&isolate, sp_ctof(*(phys_addr + i))));
				}
			}
			else
			{
				for(int i = 0; i < size; i++)
				{
					arr->Set(i, Integer::New(&isolate, *(phys_addr + i)));
				}
			}

			ctx.HeapPop(addr);

			return handle_scope.Escape(arr);
		}
	}
}
