#ifndef _INCLUDE_SOURCEPAWN_VM_API_H_
#define _INCLUDE_SOURCEPAWN_VM_API_H_

#include <stdio.h>
#include "sp_vm_types.h"
#include "sp_vm_context.h"

namespace SourcePawn
{
	class IPluginContext;

	/**
	 * Contains helper functions used by VMs and the host app
	 */
	class ISourcePawnEngine
	{
	public:
		/**
		* Loads a named file from a file pointer.  
		* Using this means base memory will be allocated by the VM.
		* 
		* @param fp		File pointer.  May be at any offset.  Not closed on return.
		* @param err		Optional error code pointer.
		* @return			A new plugin structure.
		*/	
		virtual sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err) =0;
	
		/**
		 * Loads a file from a base memory address.
		 *	
		 * @param base		Base address of the plugin's memory region.
		 * @param plugin	If NULL, a new plugin pointer is returned.
		 *                  Otherwise, the passed pointer is used.
		 * @param err		Optional error code pointer.
		 * @return			The resulting plugin pointer.
		 */
		virtual sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err) =0;

		/**
		 * Frees all of the memory associated with a plugin file.
		 * If allocated using SP_LoadFromMemory, the base and plugin pointer 
		 *  itself are not freed (so this may end up doing nothing).
		 */
		virtual int FreeFromMemory(sp_plugin_t *plugin) =0;

		/**
		 * Creates a new IContext from a context handle.
		 *
		 * @param ctx		Context to use as a basis for the IPluginContext.
		 * @return			New IPluginContext handle.
		 */
		virtual IPluginContext *CreateBaseContext(sp_context_t *ctx) =0;

		/** 
		 * Frees a context.
		 *
		 * @param ctx		Context pointer to free.
		 */
		virtual void FreeBaseContext(IPluginContext *ctx) =0;

		/**
		 * Allocates memory.
		 * 
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		virtual void *BaseAlloc(size_t size) =0;

		/**
		 * Frees memory allocated with BaseAlloc.
		 *
		 * @param mem		Memory address to free.
		 */
		virtual void BaseFree(void *memory) =0;
	};

	class ICompilation;

	class IVirtualMachine
	{
	public:
		/**
		 * Returns the string name of a VM implementation.
		 */
		virtual const char *GetVMName() =0;

		/**
		 * Begins a new compilation
		 * 
		 * @param plugin	Pointer to a plugin structure.
		 * @return			New compilation pointer.
		 */
		virtual ICompilation *StartCompilation(sp_plugin_t *plugin) =0;

		/**
		 * Sets a compilation option.
		 *
		 * @param co		Pointer to a compilation.
		 * @param key		Option key name.
		 * @param val		Option value string.
		 * @return			True if option could be set, false otherwise.
		 */
		virtual bool SetCompilationOption(ICompilation *co, const char *key, const char *val) =0;

		/**
		 * Finalizes a compilation into a new IContext.
		 * Note: This will free the ICompilation pointer.
		 *
		 * @param co		Compilation pointer.
		 * @return			New plugin context.
		 */
		virtual IPluginContext *CompileToContext(ICompilation *co) =0;

		/**
		 * Frees any internal variable usage on a context.
		 *
		 * @param ctx		Context structure pointer.
		 */
		virtual void FreeContextVars(sp_context_t *ctx) =0;

		/**
		 * Calls the "execute" function on a context.
		 *
		 * @param ctx		Executes a function in a context.
		 * @param code_idx	Index into the code section.
		 * @param result	Pointer to store result in.
		 * @return			Error code (if any).
		 */
		virtual int ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result) =0;
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_API_H_
