#ifndef _INCLUDE_SOURCEPAWN_VM_CONTEXT_H_
#define _INCLUDE_SOURCEPAWN_VM_CONTEXT_H_

#include "sp_vm_types.h"

/*****************
 ** Note that all functions return a non-zero error code on failure
 *   unless otherwise noted.
 * All input pointers must be valid unless otherwise noted as optional.
 * All output pointers on failure are undefined.
 * All local address are guaranteed to be positive.  However, they are stored
 *  as signed integers, because they must logically fit inside a cell.
 */

namespace SourcePawn
{
	class IVirtualMachine;

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
		 * @param pblic			Optionally filled with pointer to public structure.
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
		 * @param local_addr	Optionally filled with local address offset.
		 * @param phys_addr		Optionally filled with relocated physical address.
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
		 * Note that SourcePawn does not support packed strings, as such this function is
		 *  'cell to char' only.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param buffer		Destination output buffer.
		 * @param maxlength		Maximum length of output buffer, including null terminator.
		 * @param chars			Optionally filled with the number of characters written.
		 */
		virtual int LocalToString(cell_t local_addr, char *buffer, size_t maxlength, int *chars) =0;

		/**
		 * Converts a physical string to a local address.
		 * Note that SourcePawn does not support packed strings.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param chars			Number of chars to write, including NULL terminator.
		 * @param source		Source string to copy.
		 */
		virtual int StringToLocal(cell_t local_addr, size_t chars, const char *source) =0;

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
		 * @param array			Cell array to copy.
		 * @param numcells		Number of cells in the array to copy.
		 */
		virtual int PushString(cell_t *local_addr, cell_t **phys_addr, const char *string) =0;

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
		 * All natives are assigned a status of SP_NATIVE_OKAY by default.
		 * If overwrite is non-zero, already registered natives will be overwritten.
		 * 
		 * @param natives		Array of natives.
		 * @param num			Number of natives in array.
		 */
		virtual int BindNatives(sp_nativeinfo_t *natives, unsigned int num, int overwrite) =0;

		/**
		 * Binds a single native.  Overwrites any existing bind.
		 * If the context does not contain the native that will be binded the function will return
		 *  with a SP_ERROR_OT_FOUND error.
		 *
		 * @param native		Pointer to native.
		 * @param status		Status value to set (should be SP_NATIVE_OKAY).
		 */
		virtual int BindNative(sp_nativeinfo_t *native, uint32_t status) =0;

		/**
		 * Binds a single native to any non-registered or pending native.
		 * Status is automatically set to pending.
		 *
		 * @param native		Native to bind.
		 */
		virtual int BindNativeToAny(SPVM_NATIVE_FUNC native) =0;

		/**
		 * Executes a public function.
		 */
		virtual int Execute(uint32_t public_func, cell_t *result) =0;
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_CONTEXT_H_
