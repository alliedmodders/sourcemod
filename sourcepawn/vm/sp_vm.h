#ifndef _INCLUDE_SOURCEPAWN_VM_H_
#define _INCLUDE_SOURCEPAWN_VM_H_

#include <stdio.h>
#include "sp_vm_types.h"

/*****************
 ** Note that all functions return a non-zero error code on failure
 *   unless otherwise noted.
 * All input pointers must be valid unless otherwise noted as optional.
 * All output pointers on failure are undefined.
 * All local address are guaranteed to be positive.  However, they are stored
 *  as signed integers, because they must logically fit inside a cell.
 */

typedef struct sp_nativeinfo_s
{
	const char		*name;
	SPVM_NATIVE_FUNC func;
} sp_nativeinfo_t;

/**
 * Loads a named file from a file pointer.  
 * Using this means base memory will be allocated by the VM.
 * 
 * @param fp		File pointer.  May be at any offset.  Not closed on return.
 * @param err		Optional error code pointer.
 * @return			A new plugin structure.
 */
sp_plugin_t *SP_LoadFromFilePointer(FILE *fp, int *err);

/**
 * Loads a file from a base memory address.
 *
 * @param base		Base address of the plugin's memory region.
 * @param plugin	If NULL, a new plugin pointer is returned.
 *                  Otherwise, the passed pointer is used.
 * @param err		Optional error code pointer.
 * @return			The resulting plugin pointer.
 */
sp_plugin_t *SP_LoadFromMemory(void *base, sp_plugin_t *plugin, int *err);

/**
 * Frees all of the memory associated with a plugin file.
 * If allocated using SP_LoadFromMemory, the base and plugin pointer 
 *  itself are not freed (so this may end up doing nothing).
 */
int SP_FreeFromMemory(sp_plugin_t *plugin);

/**
 * Allocs memory on the secondary stack of a plugin.
 * Note that although called a heap, it is in fact a stack.
 * 
 * @param ctx			Context pointer.
 * @param cells			Number of cells to allocate.
 * @param local_adddr	Will be filled with data offset to heap.
 * @param phys_addr		Physical address to heap memory.
 */
int SP_HeapAlloc(sp_context_t *ctx, unsigned int cells, cell_t *local_addr, cell_t **phys_addr);

/**
 * Pops a heap address off the heap stack.  Use this to free memory allocated with
 *  SP_HeapAlloc().  
 * Note that in SourcePawn, the heap is in fact a bottom-up stack.  Deallocations
 *  with this native should be performed in precisely the REVERSE order.
 */
int SP_HeapPop(sp_context_t *ctx, cell_t local_addr);

/**
 * Releases a heap address using a different method than SP_HeapPop().
 * This allows you to release in any order.  However, if you allocate N 
 *  objects, release only some of them, then begin allocating again, 
 *  you cannot go back and starting freeing the originals.  
 * In other words, for each chain of allocations, if you start deallocating,
 *  then allocating more in a chain, you must only deallocate from the current 
 *  allocation chain.  This is basically SP_HeapPop() except on a larger scale.
 */
int SP_HeapRelease(sp_context_t *ctx, cell_t local_addr);

/**
 * Finds a native by name.
 *
 * @param ctx			Context pointer.
 * @param name			Name of native.
 * @param index			Optionally filled with native index number.
 */
int SP_FindNativeByName(sp_context_t *ctx, const char *name, uint32_t *index);

/**
 * Gets native info by index.
 *
 * @param ctx			Context pointer.
 * @param index			Index number of native.
 * @param native		Optionally filled with pointer to native structure.
 */
int SP_GetNativeByIndex(sp_context_t *ctx, uint32_t index, sp_native_t **native);

/**
 * Gets the number of natives.
 * 
 * @param ctx			Context pointer.
 * @param num			Filled with the number of natives.
 */
int SP_GetNativesNum(sp_context_t *ctx, uint32_t *num);

/**
 * Finds a public function by name.
 *
 * @param ctx			Context pointer.
 * @param name			Name of public
 * @param index			Optionally filled with public index number.
 */
int SP_FindPublicByName(sp_context_t *ctx, const char *name, uint32_t *index);


/**
 * Gets public function info by index.
 * 
 * @param ctx			Context pointer.
 * @param index			Public function index number.
 * @param pblic			Optionally filled with pointer to public structure.
 */
int SP_GetPublicByIndex(sp_context_t *ctx, uint32_t index, sp_public_t **pblic);

/**
 * Gets the number of public functions.
 *
 * @param ctx			Context pointer.
 * @param num			Filled with the number of public functions.
 */
int SP_GetPublicsNum(sp_context_t *ctx, uint32_t *num);

/**
 * Gets public variable info by index.
 * @param ctx			Context pointer.
 * @param index			Public variable index number.
 * @param pubvar		Optionally filled with pointer to pubvar structure.
 */
int SP_GetPubvarByIndex(sp_context_t *ctx, uint32_t index, sp_pubvar_t **pubvar);

/**
 * Finds a public variable by name.
 *
 * @param ctx			Context pointer.
 * @param name			Name of pubvar
 * @param index			Optionally filled with pubvar index number.
 * @param local_addr	Optionally filled with local address offset.
 * @param phys_addr		Optionally filled with relocated physical address.
 */
int SP_FindPubvarByName(sp_context_t *ctx, const char *name, uint32_t *index);

//:TODO: fill in the info of this function, hi
int SP_GetPubvarAddrs(sp_context_t *ctx, uint32_t index, cell_t *local_addr, cell_t **phys_addr);

/**
* Gets the number of public variables.
*
* @param ctx			Context pointer.
* @param num			Filled with the number of public variables.
*/
int SP_GetPubVarsNum(sp_context_t *ctx, uint32_t *num);

/**
 * Round-about method of converting a plugin reference to a physical address
 *
 * @param ctx			Context pointer.
 * @param local_addr	Local address in plugin.
 * @param phys_addr		Optionally filled with relocated physical address.
 */
int SP_LocalToPhysAddr(sp_context_t *ctx, cell_t local_addr, cell_t **phys_addr);

/**
 * Converts a local address to a physical string.
 * Note that SourcePawn does not support packed strings, as such this function is
 *  'cell to char' only.
 *
 * @param ctx			Context pointer.
 * @param local_addr	Local address in plugin.
 * @param chars			Optionally filled with the number of characters written.
 * @param buffer		Destination output buffer.
 * @param maxlength		Maximum length of output buffer, including null terminator.
 */
int SP_LocalToString(sp_context_t *ctx, 
					cell_t local_addr, 
					int *chars, 
					char *buffer, 
					size_t maxlength);

/**
 * Converts a physical string to a local address.
 * Note that SourcePawn does not support packed strings.
 * @param ctx			Context pointer
 * @param local_addr	Local address in plugin.
 * @param chars			Number of chars to write, including NULL terminator.
 * @param source		Source string to copy.
 */
int SP_StringToLocal(sp_context_t *ctx,
					 cell_t local_addr,
					 size_t chars,
					 const char *source);

/**
 * Pushes a cell onto the stack.  Increases the parameter count by one.
 *
 * @param ctx			Context pointer.
 * @param value			Cell value.
 */
int SP_PushCell(sp_context_t *ctx, cell_t value);

/** 
 * Pushes an array of cells onto the stack.  Increases the parameter count by one.
 * If the function returns an error it will fail entirely, releasing anything allocated in the process.
 * Note that this does not release the heap, so you should release it after
 *  calling SP_Execute().
 *
 * @param ctx			Context pointer.
 * @param local_addr	Filled with local address to release.
 * @param phys_addr		Optionally filled with physical address of new array.
 * @param array			Cell array to copy.
 * @param numcells		Number of cells in the array to copy.
 */
int SP_PushCellArray(sp_context_t *ctx, 
					 cell_t *local_addr,
					 cell_t **phys_addr,
					 cell_t array[],
					 unsigned int numcells);

/**
 * Pushes a string onto the stack (by reference) and increases the parameter count by one.
 * Note that this does not release the heap, so you should release it after
 *  calling SP_Execute().
 *
 * @param ctx			Context pointer.
 * @param local_addr	Filled with local address to release.
 * @param phys_addr		Optionally filled with physical address of new array.
 * @param array			Cell array to copy.
 * @param numcells		Number of cells in the array to copy.
 */
int SP_PushString(sp_context_t *ctx,
				  cell_t *local_addr,
				  cell_t **phys_addr,
				  const char *string);

/**
 * Individually pushes each cell of an array of cells onto the stack.  Increases the 
 *  parameter count by the number of cells pushed.
 * If the function returns an error it will fail entirely, releasing anything allocated in the process.
 *
 * @param ctx			Context pointer.
 * @param array			Array of cells to read from.
 * @param numcells		Number of cells to read.
 */
int SP_PushCellsFromArray(sp_context_t *ctx, cell_t array[], unsigned int numcells);

/** 
 * Binds a list of native names and their function pointers to a context.
 * If num is 0, the list is read until an entry with a NULL name is reached.
 * All natives are assigned a status of SP_NATIVE_OKAY by default.
 * If overwrite is non-zero, already registered natives will be overwritten.
 * 
 * @param ctx			Context pointer.
 * @param natives		Array of natives.
 * @param num			Number of natives in array.
 */
int SP_BindNatives(sp_context_t *ctx, sp_nativeinfo_t *natives, unsigned int num, int overwrite);

/**
 * Binds a single native.  Overwrites any existing bind.
 * If the context does not contain the native that will be binded the function will return
 *  with a SP_ERR_NOT_FOUND error.
 *
 * @param ctx			Context pointer.
 * @param native		Pointer to native.
 * @param status		Status value to set (should be SP_NATIVE_OKAY).
 */
int SP_BindNative(sp_context_t *ctx, sp_nativeinfo_t *native, uint32_t status);

/**
 * Binds a single native to any non-registered or pending native.
 * Status is automatically set to pending.
 *
 * @param ctx			Context pointer.
 */
int SP_BindNativeToAny(sp_context_t *ctx, SPVM_NATIVE_FUNC native);

/**
 * Executes a public function in a context.
 * The parameter count is set to zero during execution.
 * All context-specific variables that are modified are saved before execution,
 *  thus allowing nested calls to SP_Execute().  
 *
 * @param ctx			Context pointer.
 * @param idx			Public function index number.
 * @param result		Optional pointer to store return value.
 */
int SP_Execute(sp_context_t *ctx, uint32_t idx, cell_t *result);

/**
 * Creates a base context.  The context is not bound to any JIT, and thus
 *  inherits the parent code pointer of the file structure.  It does, 
 *  however, have relocated info+debug tables (even though the code address
 *  do not need to be relocated).  
 * It is guaranteed to have a newly allocated and copied memory layout
 *  of the data, heap and stack, and thus relevant address in the info/debug
 *  tables must be relocated.
 *
 * @param plugin		Plugin file structure to build a context form.
 * @param ctx			Pointer to store newly created context pointer.
 */
int SP_CreateBaseContext(sp_plugin_t *plugin, sp_context_t **ctx);

/**
 * Frees a base context.
 *
 * @param ctx			Context pointer.
 */
int SP_FreeBaseContext(sp_context_t *ctx);

//:TODO: fill in this
cell_t SP_NoExecNative(sp_context_t *ctx, cell_t *params);

#endif //_INCLUDE_SOURCEPAWN_VM_H_
