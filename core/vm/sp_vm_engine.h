#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

#include "sp_vm_api.h"

namespace SourcePawn
{
	class SourcePawnEngine : public ISourcePawnEngine
	{
	public:
		/**
		 * Loads a named file from a file pointer.  
		 * Using this means base memory will be allocated by the VM.
		 * Note: The file handle position may be undefined on entry, and is
		 *  always undefined on conclusion.
		 * 
		 * @param fp		File pointer.  May be at any offset.  Not closed on return.
		 * @param err		Optional error code pointer.
		 * @return			A new plugin structure.
		 */	
		sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err);

		/**
		 * Loads a file from a base memory address.
		 *	
		 * @param base		Base address of the plugin's memory region.
		 * @param plugin	If NULL, a new plugin pointer is returned.
		 *                  Otherwise, the passed pointer is used.
		 * @param err		Optional error code pointer.
		 * @return			The resulting plugin pointer.
		 */
		sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err);

		/**
		 * Frees all of the memory associated with a plugin file.
		 * If allocated using SP_LoadFromMemory, the base and plugin pointer 
		 *  itself are not freed (so this may end up doing nothing).
		 */
		int FreeFromMemory(sp_plugin_t *plugin);

		/**
		 * Creates a new IContext from a context handle.
		 *
		 * @param ctx		Context to use as a basis for the IPluginContext.
		 * @return			New IPluginContext handle.
		 */
		IPluginContext *CreateBaseContext(sp_context_t *ctx);

		/** 
		 * Frees a context.
		 *
		 * @param ctx		Context pointer to free.
		 */
		void FreeBaseContext(IPluginContext *ctx);

		/**
		 * Allocates memory.
		 * 
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		void *BaseAlloc(size_t size);

		/**
		 * Frees memory allocated with BaseAlloc.
		 *
		 * @param memory	Memory address to free.
		 */
		void BaseFree(void *memory);

		/**
		 * Allocates executable memory.
		 *
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		void *ExecAlloc(size_t size);

		/**
		 * Frees executable memory.
		 *
		 * @param address	Address to free.
		 */
		void ExecFree(void *address);
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
