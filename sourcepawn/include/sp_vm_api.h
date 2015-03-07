// vim: set ts=4 sw=4 tw=99 noet:
// 
// Copyright (C) 2006-2015 AlliedModders LLC
// 
// This file is part of SourcePawn. SourcePawn is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License along with
// SourcePawn. If not, see http://www.gnu.org/licenses/.
//
#ifndef _INCLUDE_SOURCEPAWN_VM_API_H_
#define _INCLUDE_SOURCEPAWN_VM_API_H_

/**
 * @file sp_vm_api.h
 * @brief Contains all of the object structures used in the SourcePawn API.
 */

#include <stdio.h>
#include <assert.h>
#include "sp_vm_types.h"

/** SourcePawn Engine API Versions */
#define SOURCEPAWN_ENGINE2_API_VERSION 0xA
#define SOURCEPAWN_API_VERSION	 0x020A

namespace SourceMod {
	struct IdentityToken_t;
};
namespace sp {
	class Environment;
};

struct sp_context_s;
typedef struct sp_context_s sp_context_t;

namespace SourcePawn
{
	class IVirtualMachine;
	class IPluginRuntime;
	class ISourcePawnEngine2;
	class ISourcePawnEnvironment;

	/* Parameter flags */
	#define SM_PARAM_COPYBACK		(1<<0)		/**< Copy an array/reference back after call */

	/* String parameter flags (separate from parameter flags) */
	#define SM_PARAM_STRING_UTF8	(1<<0)		/**< String should be UTF-8 handled */
	#define SM_PARAM_STRING_COPY	(1<<1)		/**< String should be copied into the plugin */
	#define SM_PARAM_STRING_BINARY	(1<<2)		/**< String should be handled as binary data */

	/**
	 * @brief Pseudo-NULL reference types.
	 */
	enum SP_NULL_TYPE
	{
		SP_NULL_VECTOR = 0,		/**< Float[3] reference */
		SP_NULL_STRING = 1,		/**< const String[1] reference */
	};

	/**
	 * @brief Represents what a function needs to implement in order to be callable.
	 */
	class ICallable
	{
	public:
		/**
		 * @brief Pushes a cell onto the current call.
		 *
		 * @param cell		Parameter value to push.
		 * @return			Error code, if any.
		 */
		virtual int PushCell(cell_t cell) =0;

		/**
		 * @brief Pushes a cell by reference onto the current call.
		 * NOTE: On Execute, the pointer passed will be modified if copyback is enabled.
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *		 This means you cannot push a pointer, change it, and push it again and expect
		 *       two different values to come out.
		 *
		 * @param cell		Address containing parameter value to push.
		 * @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushCellByRef(cell_t *cell, int flags=SM_PARAM_COPYBACK) =0;

		/**
		 * @brief Pushes a float onto the current call.
		 *
		 * @param number	Parameter value to push.
		 * @return			Error code, if any.
		 */
		virtual int PushFloat(float number) =0;

		/**
		 * @brief Pushes a float onto the current call by reference.
		 * NOTE: On Execute, the pointer passed will be modified if copyback is enabled.
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *		 This means you cannot push a pointer, change it, and push it again and expect
		 *       two different values to come out.
		 *
		 * @param number	Parameter value to push.
		 & @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushFloatByRef(float *number, int flags=SM_PARAM_COPYBACK) =0;

		/**
		 * @brief Pushes an array of cells onto the current call. 
		 *
		 * On Execute, the pointer passed will be modified if non-NULL and copy-back
		 * is enabled.  
		 *
		 * By reference parameters are cached and thus are not read until execution.
		 * This means you cannot push a pointer, change it, and push it again and expect
		 * two different values to come out.
		 *
		 * @param inarray	Array to copy, NULL if no initial array should be copied.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param flags		Whether or not changes should be copied back to the input array.
		 * @return			Error code, if any.
		 */
		virtual int PushArray(cell_t *inarray, unsigned int cells, int flags=0) =0;

		/**
		 * @brief Pushes a string onto the current call.
		 *
		 * @param string	String to push.
		 * @return			Error code, if any.
		 */
		virtual int PushString(const char *string) =0;

		/**
		 * @brief Pushes a string or string buffer.
		 * 
		 * NOTE: On Execute, the pointer passed will be modified if copy-back is enabled.
		 *
		 * @param buffer	Pointer to string buffer.
		 * @param length	Length of buffer.
		 * @param sz_flags	String flags.  In copy mode, the string will be copied 
		 *					according to the handling (ascii, utf-8, binary, etc).
		 * @param cp_flags	Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags) =0;

		/**
		 * @brief Cancels a function call that is being pushed but not yet executed.
		 * This can be used be reset for CallFunction() use.
		 */
		virtual void Cancel() =0;
	};


	/**
	 * @brief Encapsulates a function call in a plugin.
	 *
	 * NOTE: Function calls must be atomic to one execution context.
	 * NOTE: This object should not be deleted.  It lives for the lifetime of the plugin.
	 */
	class IPluginFunction : public ICallable
	{
	public:
		/**
		 * @brief Executes the function, resets the pushed parameter list, and 
		 * performs any copybacks.
		 *
		 * The exception state is reset upon entering and leaving this
		 * function. Callers that want to propagate exceptions from Execute()
		 * should use Invoke(). ReportError is not preferred since it would
		 * lose any custom exception messages.
		 *
		 * @param result		Pointer to store return value in.
		 * @return				Error code, if any.
		 */
		virtual int Execute(cell_t *result) =0;

		/**
		 * @brief This function is deprecated. If invoked, it reports an error.
		 *
		 * @param params		Unused.
		 * @param num_params	Unused.
		 * @param result		Unused.
		 * @return				SP_ERROR_ABORTED.
		 */
		virtual int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result) =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * @return				GetDefaultContext() of parent runtime.
		 */
		virtual IPluginContext *GetParentContext() =0;

		/**
		 * @brief Returns whether the parent plugin is paused.
		 *
		 * @return				True if runnable, false otherwise.
		 */
		virtual bool IsRunnable() =0;

		/**
		 * @brief Returns the function ID of this function.
		 * 
		 * Note: This was added in API version 4.
		 *
		 * @return				Function id.
		 */
		virtual funcid_t GetFunctionID() =0;

		/**
		 * @brief This function is deprecated. If invoked, it reports an error.
		 *
		 * @param ctx			Unused.
		 * @param result		Unused.
		 * @return				SP_ERROR_ABORTED.
		 */
		virtual int Execute2(IPluginContext *ctx, cell_t *result) =0;

		/**
		 * @brief This function is deprecated. If invoked, it reports an error.
		 *
		 * @param ctx			Unused.
		 * @param params		Unused.
		 * @param num_params	Unused.
		 * @param result		Unused.
		 * @return				SP_ERROR_ABORTED.
		 */
		virtual int CallFunction2(IPluginContext *ctx, 
			const cell_t *params, 
			unsigned int num_params, 
			cell_t *result) =0;

		/**
		 * @brief Returns parent plugin's runtime
		 *
		 * @return				IPluginRuntime pointer.
		 */
		virtual IPluginRuntime *GetParentRuntime() =0;

		/**
		 * @brief Executes the function, resets the pushed parameter list, and 
		 * performs any copybacks.
		 *
		 * Unlike Execute(), this does not reset the exception state. It is
		 * illegal to call Invoke() while an exception is unhandled. If it
		 * returns false, an exception has been thrown, and must either be
		 * handled via ExceptionHandler or propagated to its caller.
		 *
		 * @param result		Pointer to store return value in.
		 * @return				True on success, false on error.
		 */
		virtual bool Invoke(cell_t *rval = nullptr) = 0;
	};


	/**
	 * @brief Interface to managing a debug context at runtime.
	 */
	class IPluginDebugInfo
	{
	public:
		/**
		 * @brief Given a code pointer, finds the file it is associated with.
		 * 
		 * @param addr		Code address offset.
		 * @param filename	Pointer to store filename pointer in.
		 */
		virtual int LookupFile(ucell_t addr, const char **filename) =0;

		/**
		 * @brief Given a code pointer, finds the function it is associated with.
		 *
		 * @param addr		Code address offset.
		 * @param name		Pointer to store function name pointer in.
		 */
		virtual int LookupFunction(ucell_t addr, const char **name) =0;

		/**
		 * @brief Given a code pointer, finds the line it is associated with.
		 *
		 * @param addr		Code address offset.
		 * @param line		Pointer to store line number in.
		 */
		virtual int LookupLine(ucell_t addr, uint32_t *line) =0;
	};

	class ICompilation;

	/**
	 * @brief Interface to managing a runtime plugin.
	 */
	class IPluginRuntime
	{
	public:
		/**
		 * @brief Virtual destructor (you may call delete).
		 */
		virtual ~IPluginRuntime()
		{
		}

		/**
		 * @brief Returns debug info.
		 *
		 * @return				IPluginDebugInfo, or NULL if no debug info found.
		 */
		virtual IPluginDebugInfo *GetDebugInfo() =0;

		/**
		 * @brief Finds a native by name.
		 *
		 * @param name			Name of native.
		 * @param index			Optionally filled with native index number.
		 */
		virtual int FindNativeByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param index			Unused.
		 * @param native		Unused.
		 * @return		      Returns SP_ERROR_PARAM.
		 */
		virtual int GetNativeByIndex(uint32_t index, sp_native_t **native) =0;

		/**
		 * @brief Gets the number of natives.
		 * 
		 * @return				Filled with the number of natives.
		 */
		virtual uint32_t GetNativesNum() =0;

		/**
		 * @brief Finds a public function by name.
		 *
		 * @param name			Name of public
		 * @param index			Optionally filled with public index number.
		 */
		virtual int FindPublicByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Gets public function info by index.
		 * 
		 * @param index			Public function index number.
		 * @param publicptr		Optionally filled with pointer to public structure.
		 */
		virtual int GetPublicByIndex(uint32_t index, sp_public_t **publicptr) =0;

		/**
		 * @brief Gets the number of public functions.
		 *
		 * @return				Filled with the number of public functions.
		 */
		virtual uint32_t GetPublicsNum() =0;

		/**
		 * @brief Gets public variable info by index.
		 * 
		 * @param index			Public variable index number.
		 * @param pubvar		Optionally filled with pointer to pubvar structure.
		 */
		virtual int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar) =0;

		/**
		 * @brief Finds a public variable by name.
		 *
		 * @param name			Name of pubvar
		 * @param index			Optionally filled with pubvar index number.
		 */
		virtual int FindPubvarByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Gets the addresses of a public variable.
		 * 
		 * @param index			Index of public variable.
		 * @param local_addr		Address to store local address in.
		 * @param phys_addr		Address to store physically relocated in.
		 */
		virtual int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr) =0;

		/**
		 * @brief Returns the number of public variables.
		 *
		 * @return				Number of public variables.
		 */
		virtual uint32_t GetPubVarsNum() =0;

		/**
		 * @brief Returns a function by name.
		 *
		 * @param public_name		Name of the function.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionByName(const char *public_name) =0;

		/**
		 * @brief Returns a function by its id.
		 *
		 * @param func_id			Function ID.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionById(funcid_t func_id) =0;

		/**
		 * @brief Returns the default context.  The default context 
		 * should not be destroyed.
		 *
		 * @return					Default context pointer.
		 */
		virtual IPluginContext *GetDefaultContext() =0;

		/**
		 * @brief Returns true if the plugin is in debug mode.
		 *
		 * @return				True if in debug mode, false otherwise.
		 */
		virtual bool IsDebugging() =0;

		/**
		 * @brief If |co| is non-NULL, destroys |co|. No other action is taken.
		 *
		 * @return				Returns SP_ERROR_NONE.
		 */
		virtual int ApplyCompilationOptions(ICompilation *co) =0;
		
		/**
		 * @brief Sets whether or not the plugin is paused (cannot be run).
		 *
		 * @param pause			Pause state.
		 */
		virtual void SetPauseState(bool paused) =0;

		/**
		 * @brief Returns whether or not the plugin is paused (runnable).
		 *
		 * @return				Pause state (true = paused, false = not).
		 */
		virtual bool IsPaused() =0;

		/**
		 * @brief Returns the estimated memory usage of this plugin.
		 *
		 * @return				Memory usage, in bytes.
		 */
		virtual size_t GetMemUsage() =0;

		/**
		 * @brief Returns the MD5 hash of the plugin's P-Code.
		 *
		 * @return				16-byte buffer with MD5 hash of the plugin's P-Code.
		 */
		virtual unsigned char *GetCodeHash() =0;
		
		/**
		 * @brief Returns the MD5 hash of the plugin's Data.
		 *
		 * @return				16-byte buffer with MD5 hash of the plugin's Data.
		 */
		virtual unsigned char *GetDataHash() =0;

		/**
		 * @brief Update the native binding at the given index.
		 *
		 * @param pfn       Native function pointer.
		 * @param flags     Native flags.
		 * @param user      User data pointer.
		 */
		virtual int UpdateNativeBinding(uint32_t index, SPVM_NATIVE_FUNC pfn, uint32_t flags, void *data) = 0;

		/**
		 * @brief Returns the native at the given index.
		 *
		 * @param index     Native index.
		 * @return	    Native pointer, or NULL on failure.
		 */
		virtual const sp_native_t *GetNative(uint32_t index) = 0;

		/**
		 * @brief Return the file or location this plugin was loaded from.
		 */
		virtual const char *GetFilename() = 0;
	};

	/**
	 * @brief Interface to managing a context at runtime.
	 */
	class IPluginContext
	{
	public:
		/** Virtual destructor */
		virtual ~IPluginContext() { };
	public:
		/** 
		 * @brief Deprecated, does nothing.
		 *
		 * @return				NULL.
		 */
		virtual IVirtualMachine *GetVirtualMachine() =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * Returns the pointer of this object, casted to an opaque structure.
		 *
		 * @return				Returns this.
		 */
		virtual sp_context_t *GetContext() =0;

		/**
		 * @brief Returns true if the plugin is in debug mode.
		 *
		 * @return				True if in debug mode, false otherwise.
		 */
		virtual bool IsDebugging() =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param newpfn		Unused.
		 * @param oldpfn		Unused.
		 */
		virtual int SetDebugBreak(void *newpfn, void *oldpfn) =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * @return				NULL.
		 */
		virtual IPluginDebugInfo *GetDebugInfo() =0;

		/**
		 * @brief Allocates memory on the secondary stack of a plugin.
		 * Note that although called a heap, it is in fact a stack.
		 * 
		 * @param cells			Number of cells to allocate.
		 * @param local_addr	Will be filled with data offset to heap.
		 * @param phys_addr		Physical address to heap memory.
		 */
		virtual int HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr) =0;

		/**
		 * @brief Pops a heap address off the heap stack.  Use this to free memory allocated with
		 *  SP_HeapAlloc().  
		 * Note that in SourcePawn, the heap is in fact a bottom-up stack.  Deallocations
		 *  with this native should be performed in precisely the REVERSE order.
		 *
		 * @param local_addr	Local address to free.
 		 */
		virtual int HeapPop(cell_t local_addr) =0;

		/**
		 * @brief Releases a heap address using a different method than SP_HeapPop().
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
		 * @brief Deprecated, use IPluginRuntime instead.
		 *
		 * @param name			Name of native.
		 * @param index			Optionally filled with native index number.
		 */
		virtual int FindNativeByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param index			Unused.
		 * @param native		Unused.
		 * @return		      Returns SP_ERROR_PARAM.
		 */
		virtual int GetNativeByIndex(uint32_t index, sp_native_t **native) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 * 
		 * @return				Filled with the number of natives.
		 */
		virtual uint32_t GetNativesNum() =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 *
		 * @param name			Name of public
		 * @param index			Optionally filled with public index number.
		 */
		virtual int FindPublicByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 * 
		 * @param index			Public function index number.
		 * @param publicptr		Optionally filled with pointer to public structure.
		 */
		virtual int GetPublicByIndex(uint32_t index, sp_public_t **publicptr) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 *
		 * @return				Filled with the number of public functions.
		 */
		virtual uint32_t GetPublicsNum() =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 * 
		 * @param index			Public variable index number.
		 * @param pubvar		Optionally filled with pointer to pubvar structure.
		 */
		virtual int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 *
		 * @param name			Name of pubvar
		 * @param index			Optionally filled with pubvar index number.
		 */
		virtual int FindPubvarByName(const char *name, uint32_t *index) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 * 
		 * @param index			Index of public variable.
		 * @param local_addr	Address to store local address in.
		 * @param phys_addr		Address to store physically relocated in.
		 */
		virtual int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr) =0;

		/**
		 * @brief Deprecated, use IPluginRuntime instead.
		 *
		 * @return				Number of public variables.
		 */
		virtual uint32_t GetPubVarsNum() =0;

		/**
		 * @brief Converts a plugin reference to a physical address
		 *
		 * @param local_addr	Local address in plugin.
		 * @param phys_addr		Optionally filled with relocated physical address.
		 */
		virtual int LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr) =0;

		/**
		 * @brief Converts a local address to a physical string.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param addr			Destination output pointer.
		 */
		virtual int LocalToString(cell_t local_addr, char **addr) =0;

		/**
		 * @brief Converts a physical string to a local address.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param bytes			Number of chars to write, including NULL terminator.
		 * @param source		Source string to copy.
		 */
		virtual int StringToLocal(cell_t local_addr, size_t bytes, const char *source) =0;

		/**
		 * @brief Converts a physical UTF-8 string to a local address.
		 * This function is the same as the ANSI version, except it will copy the maximum number
		 * of characters possible without accidentally chopping a multi-byte character.
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
		 * @brief Deprecated, does nothing.
		 *
		 * @param value			Unused.
		 */
		virtual int PushCell(cell_t value) =0;

		/** 
		 * @brief Deprecated, does nothing.
		 *
		 * @param local_addr	Unused.
		 * @param phys_addr		Unused.
		 * @param array			Unused.
		 * @param numcells		Unused.
		 */
		virtual int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param local_addr	Unused.
		 * @param phys_addr		Unused.
		 * @param string		Unused.
		 */
		virtual int PushString(cell_t *local_addr, char **phys_addr, const char *string) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param array			Unused.
		 * @param numcells		Unused.
		 */
		virtual int PushCellsFromArray(cell_t array[], unsigned int numcells) =0;

		/** 
		 * @brief Deprecated, does nothing.
		 * 
		 * @param natives		Deprecated; do not use.
		 * @param num			Deprecated; do not use.
		 * @param overwrite		Deprecated; do not use.
		 */
		virtual int BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param native		Deprecated; do not use.
		 */
		virtual int BindNative(const sp_nativeinfo_t *native) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param native		Unused.
		 */
		virtual int BindNativeToAny(SPVM_NATIVE_FUNC native) =0;

		/**
		 * @brief Deprecated, does nothing.
		 *
		 * @param code_addr		Unused.
		 * @param result		Unused.
		 * @return				SP_ERROR_ABORTED.
		 */
		virtual int Execute(uint32_t code_addr, cell_t *result) =0;

		/**
		 * @brief Throws a error and halts any current execution.
		 *
		 * This function is deprecated. Use ReportError() instead.
		 *
		 * @param error		The error number to set.
		 * @param msg		Custom error message format.  NULL to use default.
		 * @param ...		Message format arguments, if any.
		 * @return			0 for convenience.
		 */
		virtual cell_t ThrowNativeErrorEx(int error, const char *msg, ...) =0;

		/**
		 * @brief Throws a generic native error and halts any current execution.
		 *
		 * This function is deprecated. Use ReportError() instead.
		 *
		 * @param msg		Custom error message format.  NULL to set no message.
		 * @param ...		Message format arguments, if any.
		 * @return			0 for convenience.
		 */
		virtual cell_t ThrowNativeError(const char *msg, ...) =0;

		/**
		 * @brief Returns a function by name.
		 *
		 * @param public_name		Name of the function.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionByName(const char *public_name) =0;

		/**
		 * @brief Returns a function by its id.
		 *
		 * @param func_id			Function ID.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionById(funcid_t func_id) =0;

		/**
		 * @brief Returns the identity token for this context.
		 * 
		 * Note: This is a compatibility shim and is the same as GetKey(1).
		 *
		 * @return			Identity token.
		 */
		virtual SourceMod::IdentityToken_t *GetIdentity() =0;

		/**
		 * @brief Returns a NULL reference based on one of the available NULL
		 * reference types.
		 *
		 * @param type		NULL reference type.
		 * @return			cell_t address to compare to.
		 */
		virtual cell_t *GetNullRef(SP_NULL_TYPE type) =0;

		/**
		 * @brief Converts a local address to a physical string, and allows
		 * for NULL_STRING to be set.
		 *
		 * @param local_addr	Local address in plugin.
		 * @param addr			Destination output pointer.
		 */
		virtual int LocalToStringNULL(cell_t local_addr, char **addr) =0;

		/**
		 * @brief Deprecated; do not use.
		 *
		 * @param index			Deprecated; do not use.
		 * @param native		Deprecated; do not use.
		 */
		virtual int BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC native) =0;

		/**
		 * @brief Returns if there is currently an execution in progress.
		 *
		 * @return				True if in exec, false otherwise.
		 */
		virtual bool IsInExec() =0;

		/**
		 * @brief Returns the parent runtime of a context.
		 *
		 * @return				Parent runtime.
		 */
		virtual IPluginRuntime *GetRuntime() =0;

		/**
		 * @brief This function is deprecated. If invoked, it reports an error.
		 *
		 * @param function		Unused.
		 * @param params		Unused.
		 * @param num_params	Unused.
		 * @param result		Unused.
		 * @return				SP_ERROR_ABORTED.
		 */
		virtual int Execute2(IPluginFunction *function, 
			const cell_t *params, 
			unsigned int num_params, 
			cell_t *result) =0;

		/**
		 * @brief Returns whether a context is in an error state.  
		 *
		 * This function is deprecated. Use DetectExceptions instead.
		 * 
		 * This should only be used inside natives to determine whether 
		 * a prior call failed. The return value should only be compared
		 * against SP_ERROR_NONE.
		 */
		virtual int GetLastNativeError() =0;

		/**
		 * @brief Returns the local parameter stack, starting from the 
		 * cell that contains the number of parameters passed.
		 *
		 * Local parameters are the parameters passed to the function 
		 * from which a native was called (and thus this can only be 
		 * called inside a native).
		 *
		 * @return				Parameter stack.
		 */
		virtual cell_t *GetLocalParams() =0;

		/**
		 * @brief Sets a local "key" that can be used for fast lookup.
		 *
		 * Only the "owner" of the context should be setting this.
		 *
		 * @param key			Key number (values allowed: 1 through 4).
		 * @param value			Pointer value.
		 */
		virtual void SetKey(int k, void *value) =0;

		/**
		 * @brief Retrieves a previously set "key."
		 *
		 * @param key			Key number (values allowed: 1 through 4).
		 * @param value			Pointer to store value.
		 * @return				True on success, false on failure.
		 */
		virtual bool GetKey(int k, void **value) =0;

		/**
		 * @brief If an exception is pending, this removes the exception. It
		 * is deprecated and should not be used.
		 */
		virtual void ClearLastNativeError() =0;

		/**
		 * @brief Return a pointer to the ISourcePawnEngine2 that is active.
		 * This is a convenience function.
		 *
		 * @return             API pointer.
		 */
		virtual ISourcePawnEngine2 *APIv2() = 0;

		/**
		 * @brief Report an error.
		 *
		 * @param message      Error message format.
		 * @param ...          Formatting arguments.
		 */
		virtual void ReportError(const char *fmt, ...) = 0;

		/**
		 * @brief Report an error with variadic arguments.
		 *
		 * @param message      Error message format.
		 * @param ap           Formatting arguments.
		 */
		virtual void ReportErrorVA(const char *fmt, va_list ap) = 0;

		/**
		 * @brief Report a fatal error. Fatal errors cannot be caught by any
		 * exception handler.
		 *
		 * @param message      Error message format.
		 * @param ...          Formatting arguments.
		 */
		virtual void ReportFatalError(const char *fmt, ...) = 0;

		/**
		 * @brief Report a fatal error with variadic arguments. Fatal errors
		 * cannot be caught by any exception handler.
		 *
		 * @param message      Error message format.
		 * @param ap           Formatting arguments.
		 */
		virtual void ReportFatalErrorVA(const char *fmt, va_list ap) = 0;

		/**
		 * @brief Report an error by its builtin number.
		 *
		 * @param number       Error number.
		 */
		virtual void ReportErrorNumber(int error) = 0;
	};

	/**
	 * @brief Removed.
	 */
	class IContextTrace;

	/**
	 * @brief Information about a reported error.
	 */
	class IErrorReport
	{
	public:
		/**
		 * @brief Return the message of the error report.
		 *
		 * @return          Message string.
		 */
		virtual const char *Message() const = 0;

		/**
		 * @brief True if the error is fatal and cannot be handled (though
		 * reporting can be suppressed).
		 *
		 * @return          True if fatal, false otherwise.
		 */
		virtual bool IsFatal() const = 0;

		/**
		 * @brief Return the plugin context that caused the error.
		 *
		 * @return          Plugin context.
		 */
		virtual IPluginContext *Context() const = 0;
	};

	/**
	 * @brief Allows inspecting the stack frames of the SourcePawn environment.
	 *
	 * Invoking VM functions while iterating frames will cause the iterator
	 * to become corrupt.
	 *
	 * Frames iterate in most-recent to least-recent order.
	 */
	class IFrameIterator
	{
	public:
		/**
		 * @brief Returns whether or not there are more frames to read.
		 *
		 * @return          True if there are more frames to read, false otherwise.
		 */
		virtual bool Done() const = 0;

		/**
		 * @brief Advances to the next frame.
		 *
		 * Note that the iterator starts at either a valid frame or no frame.
		 */
		virtual void Next() = 0;

		/**
		 * @brief Resets the iterator to the top of the stack.
		 */
		virtual void Reset() = 0;

		/**
		 * @brief Returns the context owning the current frame, if any.
		 *
		 * @return          Context, or null.
		 */
		virtual IPluginContext *Context() const = 0;

		/**
		 * @brief Returns whether or not the current frame is a native frame. If it
		 * is, line numbers and file paths are not available.
		 *
		 * @return          True if a native frame, false otherwise.
		 */
		virtual bool IsNativeFrame() const = 0;

		/**
		 * @brief Returns true if the frame is a scripted frame.
		 *
		 * @return          True if a scripted frame, false otherwise.
		 */
		virtual bool IsScriptedFrame() const = 0;

		/**
		 * @brief Returns the line number of the current frame, or 0 if none is
		 * available.
		 *
		 * @return          Line number on success, 0 on failure.
		 */
		virtual unsigned LineNumber() const = 0;

		/**
		 * @brief Returns the function name of the current frame, or null if
		 * none could be computed.
		 *
		 * @return          Function name on success, null on failure.
		 */
		virtual const char *FunctionName() const = 0;

		/**
		 * @brief Returns the file path of the function of the current frame,
		 * or none could be computed.
		 *
		 * @return          File path on success, null on failure.
		 */
		virtual const char *FilePath() const = 0;
	};

	/**
	 * @brief Provides callbacks for debug information.
	 */
	class IDebugListener
	{
	public:
		/**
		 * @brief No longer invoked.
		 * 
		 * @param ctx		Unused.
		 * @param error		Unused.
		 */
		virtual void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error)
		{}

		/**
		 * @brief Called on debug spew.
		 *
		 * @param msg		Message text.
		 * @param fmt		Message formatting arguments (printf-style).
		 */
		virtual void OnDebugSpew(const char *msg, ...) =0;

		/**
		 * @brief Called when an error is reported and no exception
		 * handler was available.
		 *
		 * @param report	Error report object.
		 * @param iter	    Stack frame iterator.
		 */
		virtual void ReportError(const IErrorReport &report, IFrameIterator &iter) = 0;
	};

	/**
	 * @brief Removed.
	 */
	class IProfiler;

	/**
	 * @brief Encapsulates a profiling tool that may be attached to SourcePawn.
	 */
	class IProfilingTool
	{
	public:
		/**
		 * @brief Return the name of the profiling tool.
		 *
		 * @return		  Profiling tool name.
		 */
		virtual const char *Name() = 0;

		/**
		 * @brief Description of the profiler.
		 *
		 * @return		  Description.
		 */
		virtual const char *Description() = 0;

		/**
		 * @brief Called to render help text.
		 *
		 * @param  render	   Function to render one line of text.
		 */
		virtual void RenderHelp(void (*render)(const char *fmt, ...)) = 0;
	
		/**
		 * @brief Initiate a start command.
		 *
		 * Initiate start commands through a profiling tool, returning whether
		 * or not the command is supported. If starting, SourceMod will generate
		 * events even if it cannot signal the external profiler.
		 */
		virtual bool Start() = 0;

		/**
		 * @brief Initiate a stop command.
		 *
		 * @param render	    Function to render any help messages.
		 */
		virtual void Stop(void (*render)(const char *fmt, ...)) = 0;

		/**
		 * @brief Dump profiling information.
		 *
		 * Informs the profiling tool to dump any current profiling information
		 * it has accumulated. The format and location of the output is profiling
		 * tool specific.
		 */
		virtual void Dump() = 0;
	
		/**
		 * @brief Returns whether or not the profiler is currently profiling.
		 *
		 * @return		  True if active, false otherwise.
		 */
		virtual bool IsActive() = 0;

		/**
		 * @brief Returns whether the profiler is attached.
		 *
		 * @return		  True if attached, false otherwise.
		 */
		virtual bool IsAttached() = 0;

		/**
		 * @brief Enters the scope of an event.
		 *
		 * LeaveScope() mus be called exactly once for each call to EnterScope().
		 *
		 * @param group	     A named budget group, or NULL for the default.
		 * @param name	      Event name.
		 */
		virtual void EnterScope(const char *group, const char *name) = 0;
		
		/**
		 * @brief Leave a profiling scope. This must be called exactly once for
		 * each call to EnterScope().
		 */
		virtual void LeaveScope() = 0;
	};

	struct sp_plugin_s;
	typedef struct sp_plugin_s sp_plugin_t;

	/**
	 * @brief Contains helper functions used by VMs and the host app
	 */
	class ISourcePawnEngine
	{
	public:
		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err) =0;
	
		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err) =0;

		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual int FreeFromMemory(sp_plugin_t *plugin) =0;

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
		 * @deprecated Use AllocPageMemory()
		 *
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		virtual void *ExecAlloc(size_t size) =0;

		/**
		 * @brief Frees executable memory.
		 * @deprecated Use FreePageMemory()
		 *
		 * @param address	Address to free.
		 */
		virtual void ExecFree(void *address) =0;

		/**
		 * @brief Sets the debug listener.
		 *
		 * This should be called once on application startup. It is
		 * not considered part of the userland API and may change at any time.
		 *
		 * @param listener	Pointer to an IDebugListener.
		 * @return			Old IDebugListener, or NULL if none.
		 */
		virtual IDebugListener *SetDebugListener(IDebugListener *listener) =0;
		
		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual unsigned int GetContextCallCount() =0;

		/**
		 * @brief Returns the engine API version.
		 *
		 * @return			Engine API version.
		 */
		virtual unsigned int GetEngineAPIVersion() =0;

		/**
		 * @brief Allocates executable memory.
		 *
		 * @param size		Size of memory to allocate.
		 * @return			Pointer to memory, NULL if allocation failed.
		 */
		virtual void *AllocatePageMemory(size_t size) =0;
		
		/**
		 * @brief Sets the input memory permissions to read+write.
		 *
		 * @param ptr		Memory block.
		 */
		virtual void SetReadWrite(void *ptr) =0;

		/**
		 * @brief Sets the input memory permissions to read+execute.
		 *
		 * @param ptr		Memory block.
		 */
		virtual void SetReadExecute(void *ptr) =0;

		/**
		 * @brief Frees executable memory.
		 *
		 * @param ptr		Address to free.
		 */
		virtual void FreePageMemory(void *ptr) =0;
	};

	class ExceptionHandler;

	/** 
	 * @brief Outlines the interface a Virtual Machine (JIT) must expose
	 */
	class ISourcePawnEngine2
	{
	public:
		/**
		 * @brief Returns the second engine API version.
		 *
		 * @return			API version.
		 */
		virtual unsigned int GetAPIVersion() =0;

		/**
		 * @brief Returns the string name of a VM implementation.
		 */
		virtual const char *GetEngineName() =0;

		/**
		 * @brief Returns a version string.
		 *
		 * @return			Versioning string.
		 */
		virtual const char *GetVersionString() =0;

		/**
		 * @brief Deprecated. Returns null.
		 *
		 * @return			Null.
		 */
		virtual ICompilation *StartCompilation() =0;

		/**
		 * @brief Loads a plugin from disk.
		 *
		 * If a compilation object is supplied, it is destroyed upon 
		 * the function's return.
		 * 
		 * @param co		Must be NULL.
		 * @param file		Path to the file to compile.
		 * @param err		Error code (filled on failure); required.
		 * @return		New runtime pointer, or NULL on failure.
		 */
		virtual IPluginRuntime *LoadPlugin(ICompilation *co, const char *file, int *err) =0;

		/**
		 * @brief Creates a fake native and binds it to a general callback function.
		 *
		 * @param callback	Callback function to bind the native to.
		 * @param pData		Private data to pass to the callback when the native is invoked.
		 * @return			A new fake native function as a wrapper around the callback.
		 */
		virtual SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData) =0;

		/**
		 * @brief Destroys a fake native function wrapper.  
		 *
		 * @param func		Pointer to the fake native created by CreateFakeNative.
		 */
		virtual void DestroyFakeNative(SPVM_NATIVE_FUNC func) =0;

		/**
		 * @brief Sets the debug listener.
		 *
		 * This should be called once on application startup. It is
		 * not considered part of the userland API and may change at any time.
		 *
		 * @param listener	Pointer to an IDebugListener.
		 * @return			Old IDebugListener, or NULL if none.
		 */
		virtual IDebugListener *SetDebugListener(IDebugListener *listener) =0;

		/**
		 * @brief Deprecated.
		 *
		 * @param profiler	Deprecated.
		 */
		virtual void SetProfiler(IProfiler *profiler) =0;

		/**
		 * @brief Returns the string representation of an error message.
		 *
		 * This function is deprecated and should not be used. The exception
		 * handling API should be used instead.
		 *
		 * @param err		Error code.
		 * @return			Error string, or NULL if not found.
		 */
		virtual const char *GetErrorString(int err) =0;

		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual bool Initialize() =0;
		
		/**
		 * @brief Deprecated. Does nothing.
		 */	
		virtual void Shutdown() =0;

		/**
		 * @brief Creates an empty plugin with a blob of memory.
		 *
		 * @param name		Name, for debugging (NULL for anonymous).
		 * @param bytes		Number of bytes of memory (hea+stk).
		 * @return			New runtime, or NULL if not enough memory.
		 */
		virtual IPluginRuntime *CreateEmptyRuntime(const char *name, uint32_t memory) =0;

		/**
		 * @brief Initiates the watchdog timer with the specified timeout
		 * length. This cannot be called more than once.
		 *
		 * @param timeout	Timeout, in ms.
		 * @return			True on success, false on failure.
		 */
		virtual bool InstallWatchdogTimer(size_t timeout_ms) =0;

		/**
		 * @brief Sets whether the JIT is enabled or disabled.
		 *
		 * @param enabled	True or false to enable or disable.
		 * @return			True if successful, false otherwise.
		 */
		virtual bool SetJitEnabled(bool enabled) =0;

		/**
		 * @brief Returns whether the JIT is enabled.
		 *
		 * @return			True if the JIT is enabled, false otherwise.
		 */
		virtual bool IsJitEnabled() =0;
		
		/**
		 * @brief Enables profiling. SetProfilingTool() must have been called.
		 *
		 * Note that this does not activate the profiling tool. It only enables
		 * notifications to the profiling tool. SourcePawn will send events to
		 * the profiling tool even if the tool itself is reported as inactive.
		 */
		virtual void EnableProfiling() = 0;

		/**
		 * @brief Disables profiling.
		 */
		virtual void DisableProfiling() = 0;

		/**
		 * @brief Sets the profiling tool.
		 *
		 * @param tool      Profiling tool.
		 */
		virtual void SetProfilingTool(IProfilingTool *tool) =0;

		/**
		 * @brief Loads a plugin from disk.
		 *
		 * @param file		Path to the file to compile.
		 * @param errpr		Buffer to store an error message (optional).
		 * @param maxlength	Maximum length of the error buffer.
		 * @return		New runtime pointer, or NULL on failure.
		 */
		virtual IPluginRuntime *LoadBinaryFromFile(const char *file, char *error, size_t maxlength) = 0;

		/**
		 * @brief Returns the environment.
		 */
		virtual ISourcePawnEnvironment *Environment() = 0;
	};

	// @brief This class is the v3 API for SourcePawn. It provides access to
	// the original v1 and v2 APIs as well.
	class ISourcePawnEnvironment
	{
	public:
		// The Environment must be freed with the delete keyword. This
		// automatically calls Shutdown().
		virtual ~ISourcePawnEnvironment()
		{}

		// @brief Return the API version.
		virtual int ApiVersion() = 0;

		// @brief Return a pointer to the v1 API.
		virtual ISourcePawnEngine *APIv1() = 0;

		// @brief Return a pointer to the v2 API.
		virtual ISourcePawnEngine2 *APIv2() = 0;

		// @brief Destroy the environment, releasing all resources and freeing
		// all plugin memory. This should not be called while plugins have
		// active code running on the stack.
		virtual void Shutdown() = 0;

		// @brief Enters an exception handling scope. This is intended to be
		// used on the stack and must have a corresponding call to
		// LeaveExceptionHandlingScope. When in an exception handling scope,
		// exceptions are not immediately reported. Instead the caller is
		// responsible for propagation them or clearing them.
		virtual void EnterExceptionHandlingScope(ExceptionHandler *handler) = 0;

		// @brief Leaves the most recent exception handling scope. The handler
		// is provided as a sanity check.
		virtual void LeaveExceptionHandlingScope(ExceptionHandler *handler) = 0;

		// @brief Returns whether or not an exception is currently pending.
		virtual bool HasPendingException(const ExceptionHandler *handler) = 0;

		// @brief Returns the message of the pending exception.
		virtual const char *GetPendingExceptionMessage(const ExceptionHandler *handler) = 0;
	};

	// @brief This class is the entry-point to using SourcePawn from a DLL.
	class ISourcePawnFactory
	{
	public:
		// @brief Return the API version.
		virtual int ApiVersion() = 0;

		// @brief Initializes a new environment on the current thread.
		// Currently, at most one environment may be created in a process.
		virtual ISourcePawnEnvironment *NewEnvironment() = 0;

		// @brief Returns the environment for the calling thread.
		virtual ISourcePawnEnvironment *CurrentEnvironment() = 0;
	};

	// @brief A function named "GetSourcePawnFactory" is exported from the
	// SourcePawn DLL, conforming to the following signature:
	typedef ISourcePawnFactory *(*GetSourcePawnFactoryFn)(int apiVersion);

	// @brief A helper class for handling exceptions.
	//
	// ExceptionHandlers can be used to detect, catch, and re-throw VM errors
	// within C++ code.
	// 
	// When throwing errors, SourcePawn creates an exception object. The
	// exception object is global state. As long as an exception is present,
	// all scripted code should immediately abort and return to their callers,
	// all native code should exit, all code should propagate any error states
	// until the exception is handled.
	// 
	// In some cases, an error code is not available. For example, if a native
	// detects an exception, it does not have an error status to propagate. It
	// should simply return instead. The return value will be ignored; the VM
	// knows to abort the script.
	class ExceptionHandler
	{
		friend class sp::Environment;

	public:
		ExceptionHandler(ISourcePawnEngine2 *api)
		: env_(api->Environment()),
		  catch_(true)
		{
			env_->EnterExceptionHandlingScope(this);
		}
		ExceptionHandler(IPluginContext *ctx)
		: env_(ctx->APIv2()->Environment()),
		  catch_(true)
		{
			env_->EnterExceptionHandlingScope(this);
		}
		~ExceptionHandler()
		{
			env_->LeaveExceptionHandlingScope(this);
		}

		virtual uint32_t ApiVersion() const {
			return SOURCEPAWN_API_VERSION;
		}

		// Propagates the exception instead of catching it. After calling this,
		// no more SourcePawn code should be executed until the exception is
		// handled. Callers should return immediately.
		void Rethrow() {
			assert(catch_ && HasException());
			catch_ = false;
		}

		bool HasException() const {
			return env_->HasPendingException(this);
		}

		const char *Message() const {
			return env_->GetPendingExceptionMessage(this);
		}

	private:
		// Don't allow heap construction.
		ExceptionHandler(const ExceptionHandler &other);
		void operator =(const ExceptionHandler &other);
		void *operator new(size_t size);
		void operator delete(void *, size_t);

	private:
		ISourcePawnEnvironment *env_;
		ExceptionHandler *next_;

	protected:
		// If true, the exception will be swallowed. 
		bool catch_;
	};

	// @brief An implementation of ExceptionHandler that simply collects
	// whether an exception was thrown.
	class DetectExceptions : public ExceptionHandler
	{
	public:
		DetectExceptions(ISourcePawnEngine2 *api)
		: ExceptionHandler(api) 
		{
			catch_ = false;
		}
		DetectExceptions(IPluginContext *ctx)
		: ExceptionHandler(ctx)
		{
			catch_ = false;
		}
	};
};

#endif //_INCLUDE_SOURCEPAWN_VM_API_H_
