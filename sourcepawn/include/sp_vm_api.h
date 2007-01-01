#ifndef _INCLUDE_SOURCEPAWN_VM_API_H_
#define _INCLUDE_SOURCEPAWN_VM_API_H_

#include <stdio.h>
#include "sp_vm_types.h"

#define SOURCEPAWN_VM_API_VERSION		1

namespace SourcePawn
{
	class IVirtualMachine;

	/**
	 * @brief Interface to managing a debug context at runtime.
	 */
	class IPluginDebugInfo
	{
	public:
		/**
		 * Given a code pointer, finds the file it is associated with.
		 * 
		 * @param addr		Code address offset.
		 * @param filename	Pointer to store filename pointer in.
		 */
		virtual int LookupFile(ucell_t addr, const char **filename) =0;

		/**
		 * Given a code pointer, finds the function it is associated with.
		 *
		 * @param addr		Code address offset.
		 * @param name		Pointer to store function name pointer in.
		 */
		virtual int LookupFunction(ucell_t addr, const char **name) =0;

		/**
		 * Given a code pointer, finds the line it is associated with.
		 *
		 * @param addr		Code address offset.
		 * @param line		Pointer to store line number in.
		 */
		virtual int LookupLine(ucell_t addr, uint32_t *line) =0;
	};

	/**
	 * @brief Interface to managing a context at runtime.
	 */
	class IPluginContext
	{
	public:
		virtual ~IPluginContext() { };
	public:
		/** 
		 * Returns the parent IVirtualMachine.
		 *
		 * @return				Parent virtual machine pointer.
		 */
		virtual IVirtualMachine *GetVirtualMachine() =0;

		/**
		 * Returns the child sp_context_t structure.
		 *
		 * @return				Child sp_context_t structure.
		 */
		virtual sp_context_t *GetContext() =0;

		/**
		 * Returns true if the plugin is in debug mode.
		 *
		 * @return				True if in debug mode, false otherwise.
		 */
		virtual bool IsDebugging() =0;

		/**
		 * Installs a debug break and returns the old one, if any.
		 * This will fail if the plugin is not debugging.
		 *
		 * @param newpfn		New function pointer.
		 * @param oldpfn		Pointer to retrieve old function pointer.
		 */
		virtual int SetDebugBreak(SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn) =0;

		/**
		 * Returns debug info.
		 *
		 * @return				IPluginDebugInfo, or NULL if no debug info found.
		 */
		virtual IPluginDebugInfo *GetDebugInfo() =0;

		/**
		 * Allocs memory on the secondary stack of a plugin.
		 * Note that although called a heap, it is in fact a stack.
		 * 
		 * @param cells			Number of cells to allocate.
		 * @param local_adddr	Will be filled with data offset to heap.
		 * @param phys_addr		Physical address to heap memory.
		 */
		virtual int HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr) =0;

		/**
		 * Pops a heap address off the heap stack.  Use this to free memory allocated with
		 *  SP_HeapAlloc().  
		 * Note that in SourcePawn, the heap is in fact a bottom-up stack.  Deallocations
		 *  with this native should be performed in precisely the REVERSE order.
		 *
		 * @param local_addr	Local address to free.
 		 */
		virtual int HeapPop(cell_t local_addr) =0;

		/**
		 * Releases a heap address using a different method than SP_HeapPop().
		 * This allows you to release in any order.  However, if you allocate N 
		 *  objects, release only some of them, then begin allocating again, 
		 *  you cannot go back and starting freeing the originals.  
		 * In other words, for each chain of allocations, if you start deallocating,
		 *  then allocating more in a chain, you must only deallocate from the current 
		 *  allocation chain.  This is basically HeapPop() except on a larger scale.
		 *
		 * @param local_addr	Local address to free.
 		 */
		virtual int HeapRelease(cell_t local_addr) =0;

		/**
		 * Finds a native by name.
		 *
		 * @param name			Name of native.
		 * @param index			Optionally filled with native index number.
		 */
		virtual int FindNativeByName(const char *name, uint32_t *index) =0;

		/**
		 * Gets native info by index.
		 *
		 * @param index			Index number of native.
		 * @param native		Optionally filled with pointer to native structure.
		 */
		virtual int GetNativeByIndex(uint32_t index, sp_native_t **native) =0;

		/**
		 * Gets the number of natives.
		 * 
		 * @return				Filled with the number of natives.
		 */
		virtual uint32_t GetNativesNum() =0;

		/**
		 * Finds a public function by name.
		 *
		 * @param name			Name of public
		 * @param index			Optionally filled with public index number.
		 */
		virtual int FindPublicByName(const char *name, uint32_t *index) =0;

		/**
		 * Gets public function info by index.
		 * 
		 * @param index			Public function index number.
		 * @param public		Optionally filled with pointer to public structure.
		 */
		virtual int GetPublicByIndex(uint32_t index, sp_public_t **publicptr) =0;

		/**
		 * Gets the number of public functions.
		 *
		 * @return				Filled with the number of public functions.
		 */
		virtual uint32_t GetPublicsNum() =0;

		/**
		 * Gets public variable info by index.
		 * @param index			Public variable index number.
		 * @param pubvar		Optionally filled with pointer to pubvar structure.
		 */
		virtual int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar) =0;

		/**
		 * Finds a public variable by name.
		 *
		 * @param name			Name of pubvar
		 * @param index			Optionally filled with pubvar index number.
		 */
		virtual int FindPubvarByName(const char *name, uint32_t *index) =0;

		/**
		* Gets the addresses of a public variable.
		* 
		* @param index			Index of public variable.
		* @param local_addr		Address to store local address in.
		* @param phys_addr		Address to store physically relocated in.
		*/
		virtual int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr) =0;

		/**
		 * Returns the number of public variables.
		 *
		 * @return				Number of public variables.
		 */
		virtual uint32_t GetPubVarsNum() =0;

		/**
		 * Round-about method of converting a plugin reference to a physical address
		 *
		 * @param local_addr	Local address in plugin.
		 * @param phys_addr		Optionally filled with relocated physical address.
		 */
		virtual int LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr) =0;

		/**
		 * Converts a local address to a physical string.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param addr			Destination output pointer.
		 */
		virtual int LocalToString(cell_t local_addr, char **addr) =0;

		/**
		 * Converts a physical string to a local address.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param bytes			Number of chars to write, including NULL terminator.
		 * @param source		Source string to copy.
		 */
		virtual int StringToLocal(cell_t local_addr, size_t bytes, const char *source) =0;

		/**
		* Converts a physical UTF-8 string to a local address.
		* This function is the same as the ANSI version, except it will copy the maximum number of characters possible 
		*  without accidentally chopping a multi-byte character.
		*
		* @param local_addr		Local address in plugin.
		* @param maxbytes		Number of bytes to write, including NULL terminator.
		* @param source			Source string to copy.
		* @param wrtnbytes		Optionally set to the number of actual bytes written.
		*/
		virtual int StringToLocalUTF8(cell_t local_addr, 
									  size_t maxbytes, 
									  const char *source, 
									  size_t *wrtnbytes) =0;

		/**
		 * Pushes a cell onto the stack.  Increases the parameter count by one.
		 *
		 * @param value			Cell value.
		 */
		virtual int PushCell(cell_t value) =0;

		/** 
		 * Pushes an array of cells onto the stack.  Increases the parameter count by one.
		 * If the function returns an error it will fail entirely, releasing anything allocated in the process.
		 * Note that this does not release the heap, so you should release it after
		 *  calling Execute().
		 *
		 * @param local_addr	Filled with local address to release.
		 * @param phys_addr		Optionally filled with physical address of new array.
		 * @param array			Cell array to copy.
		 * @param numcells		Number of cells in the array to copy.
		 */
		virtual int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells) =0;

		/**
		 * Pushes a string onto the stack (by reference) and increases the parameter count by one.
		 * Note that this does not release the heap, so you should release it after
		 *  calling Execute().
		 *
		 * @param local_addr	Filled with local address to release.
		 * @param phys_addr		Optionally filled with physical address of new array.
		 * @param string		Source string to push.
		 */
		virtual int PushString(cell_t *local_addr, char **phys_addr, const char *string) =0;

		/**
		 * Individually pushes each cell of an array of cells onto the stack.  Increases the 
		 *  parameter count by the number of cells pushed.
		 * If the function returns an error it will fail entirely, releasing anything allocated in the process.
		 *
		 * @param array			Array of cells to read from.
		 * @param numcells		Number of cells to read.
		 */
		virtual int PushCellsFromArray(cell_t array[], unsigned int numcells) =0;

		/** 
		 * Binds a list of native names and their function pointers to a context.
		 * If num is 0, the list is read until an entry with a NULL name is reached.
		 * If overwrite is non-zero, already registered natives will be overwritten.
		 * 
		 * @param natives		Array of natives.
		 * @param num			Number of natives in array.
		 * @param overwrite		Toggles overwrite.
		 */
		virtual int BindNatives(sp_nativeinfo_t *natives, unsigned int num, int overwrite) =0;

		/**
		 * Binds a single native.  Overwrites any existing bind.
		 * If the context does not contain the native that will be binded the function will return
		 *  with a SP_ERROR_NOT_FOUND error.
		 *
		 * @param native		Pointer to native.
		 */
		virtual int BindNative(sp_nativeinfo_t *native) =0;

		/**
		 * Binds a single native to any non-registered native.
		 *
		 * @param native		Native to bind.
		 */
		virtual int BindNativeToAny(SPVM_NATIVE_FUNC native) =0;

		/**
		 * Executes a function ID located in this context.
		 *
		 * @param funcid		Function id to execute.
		 * @param result		Pointer to store the return value (required).
		 * @return				Error code (if any) from the VM.
		 */
		virtual int Execute(uint32_t funcid, cell_t *result) =0;


		/**
		 * @brief Throws a error and halts any current execution.
		 *
		 * @param error		The error number to set.
		 * @param msg		Custom error message format.  NULL to use default.
		 * @param ...		Message format arguments, if any.
		 */
		virtual void ThrowNativeErrorEx(int error, const char *msg, ...) =0;

		/**
		 * @brief Throws a generic native error and halts any current execution.
		 *
		 * @param msg		Custom error message format.  NULL to set no message.
		 * @param ...		Message format arguments, if any.
		 * @return			0 for convenience.
		 */
		virtual cell_t ThrowNativeError(const char *msg, ...) =0;
	};


	/**
	 * @brief Information about a position in a call stack.
	 */
	struct CallStackInfo
	{
		const char *filename;		/* NULL if not found */
		unsigned int line;			/* 0 if not found */
		const char *function;		/* NULL if not found */
	};

	/**
	 * @brief Retrieves error information from a debug hook.
	 */
	class IContextTrace
	{
	public:
		/**
		 * @brief Returns the integer error code.
		 *
		 * @return			Integer error code.
		 */
		virtual int GetErrorCode() =0;

		/**
		 * @brief Returns a string describing the error.
		 *
		 * @return			Error string.
		 */
		virtual const char *GetErrorString() =0;

		/**
		 * @brief Returns whether debug info is available.
		 *
		 * @return			True if debug info is available, false otherwise.
		 */
		virtual bool DebugInfoAvailable() =0;

		/**
		 * @brief Returns a custom error message.
		 *
		 * @return			A pointer to a custom error message, or NULL otherwise.
		 */
		virtual const char *GetCustomErrorString() =0;

		/**
		 * @brief Returns trace info for a specific point in the backtrace, if any.
		 * The next subsequent call to GetTraceInfo() will return the next item in the call stack.
		 * Calls are retrieved in descending order (i.e. the first item is at the top of the stack/call sequence).
		 *
		 * @param trace		An ErrorTraceInfo buffer to store information (NULL to ignore).
		 * @return			True if successful, false if there are no more traces.
		 */
		virtual bool GetTraceInfo(CallStackInfo *trace) =0;

		/**
		 * @brief Resets the trace to its original position (the call on the top of the stack).
		 */
		virtual void ResetTrace() =0;

		/**
		 * @brief Retrieves the name of the last native called.
		 * Returns NULL if there was no native that caused the error.
		 *
		 * @param index		Opterional pointer to store index.
		 * @return			Native name, or NULL if none.
		 */
		virtual const char *GetLastNative(uint32_t *index) =0;
	};


	/**
	 * @brief Provides callbacks for debug information.
	 */
	class IDebugListener
	{
	public:
		virtual void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error) =0;
	};


	/**
	 * @brief Contains helper functions used by VMs and the host app
	 */
	class ISourcePawnEngine
	{
	public:
		/**
		 * @brief Loads a named file from a file pointer.  
		 * Note: Using this means the memory will be allocated by the VM.
		 * 
		 * @param fp			File pointer.  May be at any offset.  Not closed on return.
		 * @param err		Optional error code pointer.
		 * @return			A new plugin structure.
		 */	
		virtual sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err) =0;
	
		/**
		 * @brief Loads a file from a base memory address.
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
		 * @brief Creates a new IContext from a context handle.
		 *
		 * @param ctx		Context to use as a basis for the IPluginContext.
		 * @return			New IPluginContext handle.
		 */
		virtual IPluginContext *CreateBaseContext(sp_context_t *ctx) =0;

		/** 
		 * @brief Frees a base context.  Does not free the sp_context_t it holds.
		 *
		 * @param ctx		Context pointer to free.
		 */
		virtual void FreeBaseContext(IPluginContext *ctx) =0;

		/**
		 * @brief Allocates large blocks of temporary memory.
		 * 
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		virtual void *BaseAlloc(size_t size) =0;

		/**
		 * @brief Frees memory allocated with BaseAlloc.
		 *
		 * @param memory	Memory address to free.
		 */
		virtual void BaseFree(void *memory) =0;

		/**
		 * @brief Allocates executable memory.
		 *
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		virtual void *ExecAlloc(size_t size) =0;

		/**
		 * @brief Frees executable memory.
		 *
		 * @param address	Address to free.
		 */
		virtual void ExecFree(void *address) =0;

		/**
		 * @brief Sets the debug listener.  This should only be called once.
		 * If called successively (using manual chaining), only the last function should
		 * attempt to call back into the same plugin.  Otherwise, globally cached states
		 * can be accidentally overwritten.
		 *
		 * @param listener	Pointer to an IDebugListener.
		 * @return			Old IDebugListener, or NULL if none.
		 */
		virtual IDebugListener *SetDebugListener(IDebugListener *pListener) =0;
		
		/**
		 * @brief Returns the number of plugins on the call stack.
		 *
		 * @return			Number of contexts in the call stack.
		 */
		virtual unsigned int GetContextCallCount() =0;
	};


	/**
	 * @brief Dummy class for encapsulating private compilation data.
	 */
	class ICompilation
	{
	public:
		virtual ~ICompilation() { };
	};


	/** 
	 * @brief Outlines the interface a Virtual Machine (JIT) must expose
	 */
	class IVirtualMachine
	{
	public:
		/**
		 * @brief Returns the current API version.
		 */
		virtual unsigned int GetAPIVersion() =0;

		/**
		 * @brief Returns the string name of a VM implementation.
		 */
		virtual const char *GetVMName() =0;

		/**
		 * @brief Begins a new compilation
		 * 
		 * @param plugin	Pointer to a plugin structure.
		 * @return			New compilation pointer.
		 */
		virtual ICompilation *StartCompilation(sp_plugin_t *plugin) =0;

		/**
		 * @brief Sets a compilation option.
		 *
		 * @param co		Pointer to a compilation.
		 * @param key		Option key name.
		 * @param val		Option value string.
		 * @return			True if option could be set, false otherwise.
		 */
		virtual bool SetCompilationOption(ICompilation *co, const char *key, const char *val) =0;

		/**
		 * @brief Finalizes a compilation into a new sp_context_t.
		 * Note: This will free the ICompilation pointer.
		 *
		 * @param co		Compilation pointer.
		 * @param err		Filled with error code on exit.
		 * @return			New plugin context.
		 */
		virtual sp_context_t *CompileToContext(ICompilation *co, int *err) =0;

		/**
		 * @brief Aborts a compilation and frees the ICompilation pointer.
		 *
		 * @param co		Compilation pointer.
		 */
		virtual void AbortCompilation(ICompilation *co) =0;

		/**
		 * @brief Frees any internal variable usage on a context.
		 *
		 * @param ctx		Context structure pointer.
		 */
		virtual void FreeContext(sp_context_t *ctx) =0;

		/**
		 * @brief Calls the "execute" function on a context.
		 *
		 * @param ctx		Executes a function in a context.
		 * @param code_addr	Index into the code section.
		 * @param result	Pointer to store result into.
		 * @return			Error code (if any).
		 */
		virtual int ContextExecute(sp_context_t *ctx, uint32_t code_addr, cell_t *result) =0;

		/**
		 * @brief Given a context and a code address, returns the index of the function.
		 * 
		 * @param ctx		Context to search.
		 * @param code_addr	Index into the code section.
		 * @param result	Pointer to store result into.
		 * @return			True if code index is valid, false otherwise.
		 */
		virtual bool FunctionLookup(const sp_context_t *ctx, uint32_t code_addr, unsigned int *result) =0;

		/**
		 * @brief Returns the number of functions defined in the context.
		 *
		 * @param ctx		Context to search.
		 * @return			Number of functions.
		 */
		virtual unsigned int FunctionCount(const sp_context_t *ctx) =0;
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_API_H_
