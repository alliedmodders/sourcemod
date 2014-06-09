#ifndef _INCLUDE_V8_MARSHALLER_H_
#define _INCLUDE_V8_MARSHALLER_H_

#include "SPAPIEmulation.h"
#include <v8.h>
#include <stack>
#include <string>

namespace SMV8
{
	namespace SPEmulation
	{
		using namespace v8;

		struct ReferenceInfo
		{
			Persistent<Object> refObj;
			cell_t addr;
			bool forcefloat;
		};

		class V8ToSPMarshaller
		{
		public:
			V8ToSPMarshaller(Isolate& isolate, NativeData& native);
			virtual ~V8ToSPMarshaller();
			Local<Value> HandleNativeCall(const FunctionCallbackInfo<Value>& info);
		private:
			void PushParam(Local<Value> val, cell_t* param_dst, bool forcefloat);
			void PushObject(Local<Object> val, cell_t* param_dst, bool forcefloat);
			void PushInt(Local<Integer> val, cell_t* param_dst);
			void PushFloat(Local<Number> val, cell_t* param_dst);
			void PushBool(Local<Boolean> val, cell_t* param_dst);
			void PushByRef(Local<Object> val, cell_t* param_dst, bool forcefloat);
			void PushArray(Local<Array> val, Local<Object> refObj, cell_t* param_dst, bool forcefloat);
			void PushString(Local<String> val, Local<Object> refObj, cell_t* param_dst);
			void PushFunction(Local<Function> val, cell_t* param_dst);
			Local<Object> WrapInDummyObject(Local<Value> val);
			Local<Value> GetResult(cell_t result);
			Local<Value> PopRef(ReferenceInfo *ri);
			Local<Integer> PopIntRef(ReferenceInfo *ri);
			Local<Number> PopFloatRef(ReferenceInfo *ri);
			Local<String> PopString(ReferenceInfo *ri);
			Local<Array> PopArray(ReferenceInfo *ri);
			void CopyBackRefs();
		private:
			Isolate& isolate;
			NativeData& native;
			PluginRuntime& runtime;
			IPluginContext& ctx;
			std::stack<ReferenceInfo*> refs;
		};
	}
}

#endif // !_INCLUDE_V8_MARSHALLER_H_
