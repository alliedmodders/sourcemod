#include "SPAPIEmulation.h"
#include <cstdlib>

namespace SMV8
{
	namespace SPEmulation
	{
		PluginContext::PluginContext(PluginRuntime* parentRuntime)
			: parentRuntime(parentRuntime), inExec(false)
		{
			ClearLastNativeError();
			heap = new char[4096];
			hp = heap;
		}

		PluginContext::~PluginContext()
		{
			HeapRelease(0);
			delete [] heap;
		}

		IVirtualMachine *PluginContext::GetVirtualMachine()
		{
			return NULL;
		}

		sp_context_t *PluginContext::GetContext()
		{
			return reinterpret_cast<sp_context_t *>(this);
		}

		bool PluginContext::IsDebugging()
		{
			return true;
		}

		int PluginContext::SetDebugBreak(void *newpfn, void *oldpfn)
		{
			return SP_ERROR_ABORTED;
		}

		IPluginDebugInfo *PluginContext::GetDebugInfo()
		{
			return NULL;
		}

		/* "heap" management -_-' */
		int PluginContext::HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr)
		{
			CheckHeapSize(cells + 1);

			cell_t *size = (cell_t*)hp;
			hp += sizeof(cell_t);
			*size = cells;

			*phys_addr = (cell_t*)hp;
			*local_addr = hp - heap;
			hp += cells * sizeof(cell_t);

			return SP_ERROR_NONE;
		}

		int PluginContext::HeapPop(cell_t local_addr)
		{
			if(local_addr < 0 || local_addr >= hp - heap)
			{
				return SP_ERROR_INVALID_ADDRESS;
			}

			char *phys_addr = heap + local_addr;
			cell_t *size = (cell_t*)phys_addr - 1;
			if(static_cast<unsigned int>(*size) * sizeof(cell_t) != hp - phys_addr)
			{
				return SP_ERROR_INVALID_ADDRESS;
			}

			hp = (char*)size;

			return SP_ERROR_NONE;
		}

		int PluginContext::HeapRelease(cell_t local_addr)
		{
			if(local_addr < 0 || local_addr >= hp - heap)
			{
				return SP_ERROR_INVALID_ADDRESS;
			}

			char *phys_addr = heap + local_addr;
			cell_t *size = (cell_t*)phys_addr - 1;

			hp = (char*)size;

			return SP_ERROR_NONE;
		}

		void PluginContext::CheckHeapSize(unsigned int cells)
		{
			unsigned memUsed = hp - heap;
			while(cells * sizeof(cell_t) > heapSize - memUsed)
			{
				unsigned int newHeapSize = heapSize * 2;
				char *newHeap = new char[newHeapSize];

				// Possibly change to cellsUsed.
				memcpy(newHeap, heap, heapSize);

				delete [] heap;
				heap = newHeap;
				hp = newHeap + memUsed;
				heapSize = newHeapSize;
			}
		}
		
		// Why the hell are these not only in the IPluginRuntime if you can just get the runtime with GetRuntime...
		IPluginFunction *PluginContext::GetFunctionById(funcid_t func_id)
		{
			return parentRuntime->GetFunctionById(func_id);
		}

		IPluginFunction *PluginContext::GetFunctionByName(const char *public_name)
		{
			return parentRuntime->GetFunctionByName(public_name);
		}

		int PluginContext::FindNativeByName(const char *name, uint32_t *index)
		{
			return parentRuntime->FindNativeByName(name, index);
		}

		int PluginContext::GetNativeByIndex(uint32_t index, sp_native_t **native)
		{
			return parentRuntime->GetNativeByIndex(index, native);
		}

		uint32_t PluginContext::GetNativesNum()
		{
			return parentRuntime->GetNativesNum();
		}

		int PluginContext::FindPublicByName(const char *name, uint32_t *index)
		{
			return parentRuntime->FindPublicByName(name, index);
		}

		int PluginContext::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
		{
			return parentRuntime->GetPublicByIndex(index, pblic);
		}

		uint32_t PluginContext::GetPublicsNum()
		{
			return parentRuntime->GetPublicsNum();
		}

		int PluginContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
		{
			return parentRuntime->GetPubvarByIndex(index, pubvar);
		}

		int PluginContext::FindPubvarByName(const char *name, uint32_t *index)
		{
			return parentRuntime->FindPubvarByName(name, index);
		}

		int PluginContext::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
		{
			return parentRuntime->GetPubvarAddrs(index, local_addr, phys_addr);
		}

		uint32_t PluginContext::GetPubVarsNum()
		{
			return parentRuntime->GetPubVarsNum();
		}

		int PluginContext::LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr)
		{
			if(local_addr < 0 || local_addr >= hp - heap)
			{
				return SP_ERROR_INVALID_ADDRESS;
			}

			if(phys_addr)
			{
				*phys_addr = (cell_t *)(heap + local_addr);
			}

			return SP_ERROR_NONE;
		}

		int PluginContext::LocalToString(cell_t local_addr, char **addr)
		{
			if(local_addr < 0 || local_addr >= hp - heap)
			{
				return SP_ERROR_INVALID_ADDRESS;
			}

			*addr = heap + local_addr;

			return SP_ERROR_NONE;
		}

		int PluginContext::StringToLocal(cell_t local_addr, size_t bytes, const char *source)
		{
			char *dest;
			int err;

			if((err = LocalToString(local_addr, &dest)) != SP_ERROR_NONE)
				return err;

			size_t len = strlen(source);
			if(len >= bytes)
			{
				len = bytes - 1;
			}

			memmove(dest, source, len);
			dest[len] = '\0';

			return SP_ERROR_NONE;
		}

		inline int __CheckValidChar(char *c)
		{
			int count;
			int bytecount = 0;

			for (count=1; (*c & 0xC0) == 0x80; count++)
			{
				c--;
			}

			switch (*c & 0xF0)
			{
			case 0xC0:
			case 0xD0:
				{
					bytecount = 2;
					break;
				}
			case 0xE0:
				{
					bytecount = 3;
					break;
				}
			case 0xF0:
				{
					bytecount = 4;
					break;
				}
			}

			if (bytecount != count)
			{
				return count;
			}

			return 0;
		}

		int PluginContext::StringToLocalUTF8(cell_t local_addr, 
										size_t maxbytes, 
										const char *source, 
										size_t *wrtnbytes)
		{
			char *dest;
			bool needtocheck;
			int err;

			if((err = LocalToString(local_addr, &dest)) != SP_ERROR_NONE)
				return err;

			size_t len = strlen(source);
			if(len >= maxbytes)
			{
				len = maxbytes - 1;
				needtocheck = true;
			}

			memmove(dest, source, len);
			if ((dest[len-1] & 1<<7) && needtocheck)
			{
				len -= __CheckValidChar(dest+len-1);
			}
			dest[len] = '\0';

			if (wrtnbytes)
			{
				*wrtnbytes = len;
			}
	
			return SP_ERROR_NONE;
		}

		int PluginContext::PushCell(cell_t value)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::PushString(cell_t *local_addr, char **phys_addr, const char *string)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::PushCellsFromArray(cell_t array[], unsigned int numcells)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::BindNative(const sp_nativeinfo_t *native)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
		{
			return SP_ERROR_ABORTED;
		}

		int PluginContext::Execute(uint32_t code_addr, cell_t *result)
		{
			return SP_ERROR_ABORTED;
		}

		cell_t PluginContext::ThrowNativeErrorEx(int error, const char *msg, ...)
		{
			va_list args;
			va_start(args, msg);
			cell_t res = ThrowNativeErrorEx(error, msg, args);
			va_end(args);
			return res;
		}

		cell_t PluginContext::ThrowNativeError(const char *msg, ...)
		{
			va_list args;
			va_start(args, msg);
			cell_t res = ThrowNativeError(msg, args);
			va_end(args);
			return res;
		}

		cell_t PluginContext::ThrowNativeErrorEx(int error, const char *msg, va_list args)
		{
			if(!inExec)
			{
				return 0;
			}

			nativeError = error;

			if(msg)
			{
				errMessage = msg;
			}

			return SP_ERROR_NONE;
		}

		cell_t PluginContext::ThrowNativeError(const char *msg, va_list args)
		{
			return ThrowNativeErrorEx(SP_ERROR_NATIVE, msg, args);
		}

		SourceMod::IdentityToken_t *PluginContext::GetIdentity()
		{
			SourceMod::IdentityToken_t *tok;

			if (GetKey(1, (void **)&tok))
			{
				return tok;
			}

			return NULL;
		}

		cell_t *PluginContext::GetNullRef(SP_NULL_TYPE type)
		{
			// NULL has already been resolved by the marshalling system.
			// Any JS null references have been turned into NULL
			return NULL;
		}

		int PluginContext::LocalToStringNULL(cell_t local_addr, char **addr)
		{
			// NULL has already been resolved by the marshalling system.
			return LocalToString(local_addr,addr);
		}

		int PluginContext::BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC native)
		{
			return SP_ERROR_ABORTED;
		}

		bool PluginContext::IsInExec()
		{
			return inExec;
		}

		IPluginRuntime *PluginContext::GetRuntime()
		{
			return parentRuntime;
		}

		int PluginContext::Execute2(IPluginFunction *function, 
			const cell_t *params, 
			unsigned int num_params,
			cell_t *result)
		{
	/*		bool savedInExec = inExec;
			inExec = true;



			inExec = savedInExec;*/
			return SP_ERROR_ABORTED;
		}

		Handle<Value> PluginContext::ExecuteV8(IPluginFunction *function, int argc, Handle<Value> argv[])
		{
			bool savedInExec = inExec;
			inExec = true;

			HandleScope handle_scope(parentRuntime->GetIsolate());
			Handle<Value> res = parentRuntime->CallV8Function(function->GetFunctionID(), argc, argv);

			inExec = savedInExec;
			return handle_scope.Close(res);
		}

		int PluginContext::GetLastNativeError()
		{
			return nativeError;
		}

		cell_t *PluginContext::GetLocalParams()
		{
			// TODO: Implement this ugly hack
			return NULL;
		}

		void PluginContext::SetKey(int k, void *value)
		{
			if (k < 1 || k > 4)
			{
				return;
			}

			keys[k - 1] = value;
			keys_set[k - 1] = true;
		}

		bool PluginContext::GetKey(int k, void **value)
		{
			if (k < 1 || k > 4 || keys_set[k - 1] == false)
			{
				return false;
			}

			*value = keys[k - 1];

			return true;
		}

		void PluginContext::ClearLastNativeError()
		{
			nativeError = SP_ERROR_NONE;
		}
	}
}
