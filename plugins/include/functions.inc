/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#if defined _functions_included
 #endinput
#endif
#define _functions_included

#define SP_PARAMFLAG_BYREF  (1<<0)  /**< Internal use only. */

/**
 * Describes the various ways to pass parameters to functions or forwards.
 */
enum ParamType
{
	Param_Any           = 0,                            /**< Any data type can be pushed */
	Param_Cell          = (1<<1),                       /**< Only basic cells can be pushed */
	Param_Float         = (2<<1),                       /**< Only floats can be pushed */
	Param_String        = (3<<1)|SP_PARAMFLAG_BYREF,    /**< Only strings can be pushed */
	Param_Array         = (4<<1)|SP_PARAMFLAG_BYREF,    /**< Only arrays can be pushed */
	Param_CellByRef     = (1<<1)|SP_PARAMFLAG_BYREF,    /**< Only a cell by reference can be pushed */
	Param_FloatByRef    = (2<<1)|SP_PARAMFLAG_BYREF,    /**< Only a float by reference can be pushed */
	Param_Int64         = (5<<1),                       /**< Only 64-bit integers can be pushed */
	Param_SMAddress     = (6<<1)                        /**< Only Addresses can be pushed */
};

/**
 * Defines how a forward iterates through plugin functions.
 */
enum ExecType
{
	ET_Ignore   = 0,    /**< Ignore all return values, return 0 */
	ET_Single   = 1,    /**< Only return the last exec, ignore all others */
	ET_Event    = 2,    /**< Acts as an event with the Actions defined in core.inc, no mid-Stops allowed, returns highest */
	ET_Hook     = 3     /**< Acts as a hook with the Actions defined in core.inc, mid-Stops allowed, returns highest */
};

/**
 * @section Flags that are used with Call_PushArrayEx() and Call_PushStringEx()
 */

#define SM_PARAM_COPYBACK       (1<<0)      /**< Copy an array/reference back after call */

#define SM_PARAM_STRING_UTF8    (1<<0)      /**< String should be UTF-8 handled */
#define SM_PARAM_STRING_COPY    (1<<1)      /**< String should be copied into the plugin */
#define SM_PARAM_STRING_BINARY  (1<<2)      /**< Treat the string as a binary string */

/**
 * @endsection
 */

/**
 * @section Error codes
 */
#define SP_ERROR_NONE                   0   /**< No error occurred */
#define SP_ERROR_FILE_FORMAT            1   /**< File format unrecognized */
#define SP_ERROR_DECOMPRESSOR           2   /**< A decompressor was not found */
#define SP_ERROR_HEAPLOW                3   /**< Not enough space left on the heap */
#define SP_ERROR_PARAM                  4   /**< Invalid parameter or parameter type */
#define SP_ERROR_INVALID_ADDRESS        5   /**< A memory address was not valid */
#define SP_ERROR_NOT_FOUND              6   /**< The object in question was not found */
#define SP_ERROR_INDEX                  7   /**< Invalid index parameter */
#define SP_ERROR_STACKLOW               8   /**< Not enough space left on the stack */
#define SP_ERROR_NOTDEBUGGING           9   /**< Debug mode was not on or debug section not found */
#define SP_ERROR_INVALID_INSTRUCTION    10  /**< Invalid instruction was encountered */
#define SP_ERROR_MEMACCESS              11  /**< Invalid memory access */
#define SP_ERROR_STACKMIN               12  /**< Stack went beyond its minimum value */
#define SP_ERROR_HEAPMIN                13  /**< Heap went beyond its minimum value */
#define SP_ERROR_DIVIDE_BY_ZERO         14  /**< Division by zero */
#define SP_ERROR_ARRAY_BOUNDS           15  /**< Array index is out of bounds */
#define SP_ERROR_INSTRUCTION_PARAM      16  /**< Instruction had an invalid parameter */
#define SP_ERROR_STACKLEAK              17  /**< A native leaked an item on the stack */
#define SP_ERROR_HEAPLEAK               18  /**< A native leaked an item on the heap */
#define SP_ERROR_ARRAY_TOO_BIG          19  /**< A dynamic array is too big */
#define SP_ERROR_TRACKER_BOUNDS         20  /**< Tracker stack is out of bounds */
#define SP_ERROR_INVALID_NATIVE         21  /**< Native was pending or invalid */
#define SP_ERROR_PARAMS_MAX             22  /**< Maximum number of parameters reached */
#define SP_ERROR_NATIVE                 23  /**< Error originates from a native */
#define SP_ERROR_NOT_RUNNABLE           24  /**< Function or plugin is not runnable */
#define SP_ERROR_ABORTED                25  /**< Function call was aborted */

/**
 * @endsection
 */

methodmap GlobalForward < Handle {
	// Creates a global forward.
	//
	// @note The name used to create the forward is used as its public function in all target plugins.
	// @note This is ideal for global, static forwards that are never changed.
	// @note Global forwards cannot be cloned.
	// @note Use CloseHandle() to destroy these.
	//
	// @param name              Name of public function to use in forward.
	// @param type              Execution type to be used.
	// @param ...               Variable number of parameter types (up to 32).
	// @return                  Handle to new global forward.
	// @error                   More than 32 parameter types passed.
	public native GlobalForward(const char[] name, ExecType type, ParamType ...);

	// Returns the number of functions in a global or private forward's call list.
	property int FunctionCount {
		public native get();
	}
};

methodmap PrivateForward < GlobalForward {
	// Creates a private forward.
	//
	// @note No functions are automatically added. Use AddToForward() to do this.
	// @note Private forwards can be cloned.
	// @note Use CloseHandle() to destroy these.
	//
	// @param type          Execution type to be used.
	// @param ...           Variable number of parameter types (up to 32).
	// @return              Handle to new private forward.
	// @error               More than 32 parameter types passed.
	public native PrivateForward(ExecType type, ParamType ...);

	// Adds a function to a private forward's call list.
	//
	// @note Cannot be used during an incomplete call.
	//
	// @param plugin        Handle of the plugin that contains the function.
	//                      Pass INVALID_HANDLE to specify the calling plugin.
	// @param func          Function to add to forward.
	// @return              True on success, false otherwise.
	// @error               Invalid or corrupt private forward handle, invalid or corrupt plugin handle, or invalid function.
	public native bool AddFunction(Handle plugin, Function func);

	// Removes a function from a private forward's call list.
	//
	// @note Only removes one instance.
	// @note Functions will be removed automatically if their parent plugin is unloaded.
	//
	// @param plugin        Handle of the plugin that contains the function.
	//                      Pass INVALID_HANDLE to specify the calling plugin.
	// @param func          Function to remove from forward.
	// @return              True on success, false otherwise.
	// @error               Invalid or corrupt private forward handle, invalid or corrupt plugin handle, or invalid function.
	public native bool RemoveFunction(Handle plugin, Function func);

	// Removes all instances of a plugin from a private forward's call list.
	//
	// @note Functions will be removed automatically if their parent plugin is unloaded.
	//
	// @param plugin        Handle of the plugin to remove instances of.
	//                      Pass INVALID_HANDLE to specify the calling plugin.
	// @return              Number of functions removed from forward.
	// @error               Invalid or corrupt private forward handle or invalid or corrupt plugin handle.
	public native int RemoveAllFunctions(Handle plugin);
};

/**
 * Gets a function id from a function name.
 *
 * @param plugin        Handle of the plugin that contains the function.
 *                      Pass INVALID_HANDLE to search in the calling plugin.
 * @param name          Name of the function.
 * @return              Function id or INVALID_FUNCTION if not found.
 * @error               Invalid or corrupt plugin handle.
 */
native Function GetFunctionByName(Handle plugin, const char[] name);

/**
 * Creates a global forward.
 *
 * @note The name used to create the forward is used as its public function in all target plugins.
 * @note This is ideal for global, static forwards that are never changed.
 * @note Global forwards cannot be cloned.
 * @note Use CloseHandle() to destroy these.
 *
 * @param name          Name of public function to use in forward.
 * @param type          Execution type to be used.
 * @param ...           Variable number of parameter types (up to 32).
 * @return              Handle to new global forward.
 * @error               More than 32 parameter types passed.
 */
native GlobalForward CreateGlobalForward(const char[] name, ExecType type, ParamType ...);

/**
 * Creates a private forward.
 *
 * @note No functions are automatically added. Use AddToForward() to do this.
 * @note Private forwards can be cloned.
 * @note Use CloseHandle() to destroy these.
 *
 * @param type          Execution type to be used.
 * @param ...           Variable number of parameter types (up to 32).
 * @return              Handle to new private forward.
 * @error               More than 32 parameter types passed.
 */
native PrivateForward CreateForward(ExecType type, ParamType ...);

/**
 * Returns the number of functions in a global or private forward's call list.
 *
 * @param fwd           Handle to global or private forward.
 * @return              Number of functions in forward.
 * @error               Invalid or corrupt forward handle.
 */
native int GetForwardFunctionCount(Handle fwd);

/**
 * Adds a function to a private forward's call list.
 *
 * @note Cannot be used during an incomplete call.
 *
 * @param fwd           Handle to private forward.
 * @param plugin        Handle of the plugin that contains the function.
 *                      Pass INVALID_HANDLE to specify the calling plugin.
 * @param func          Function to add to forward.
 * @return              True on success, false otherwise.
 * @error               Invalid or corrupt private forward handle, invalid or corrupt plugin handle, or invalid function.
 */
native bool AddToForward(Handle fwd, Handle plugin, Function func);

/**
 * Removes a function from a private forward's call list.
 *
 * @note Only removes one instance.
 * @note Functions will be removed automatically if their parent plugin is unloaded.
 *
 * @param fwd           Handle to private forward.
 * @param plugin        Handle of the plugin that contains the function.
 *                      Pass INVALID_HANDLE to specify the calling plugin.
 * @param func          Function to remove from forward.
 * @return              True on success, false otherwise.
 * @error               Invalid or corrupt private forward handle, invalid or corrupt plugin handle, or invalid function.
 */
native bool RemoveFromForward(Handle fwd, Handle plugin, Function func);

/**
 * Removes all instances of a plugin from a private forward's call list.
 *
 * @note Functions will be removed automatically if their parent plugin is unloaded.
 *
 * @param fwd           Handle to private forward.
 * @param plugin        Handle of the plugin to remove instances of.
 *                      Pass INVALID_HANDLE to specify the calling plugin.
 * @return              Number of functions removed from forward.
 * @error               Invalid or corrupt private forward handle or invalid or corrupt plugin handle.
 */
native int RemoveAllFromForward(Handle fwd, Handle plugin);

/**
 * Starts a call to functions in a forward's call list.
 *
 * @note Cannot be used during an incomplete call.
 *
 * @param fwd           Handle to global or private forward.
 * @error               Invalid or corrupt forward handle or called before another call has completed.
 */
native void Call_StartForward(Handle fwd);

/**
 * Starts a call to a function.
 *
 * @note Cannot be used during an incomplete call.
 *
 * @param plugin        Handle of the plugin that contains the function.
 *                      Pass INVALID_HANDLE to specify the calling plugin.
 * @param func          Function to call.
 * @error               Invalid or corrupt plugin handle, invalid function, or called before another call has completed.
 */
native void Call_StartFunction(Handle plugin, Function func);

/**
 * Pushes a cell onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         Cell value to push.
 * @error               Called before a call has been started.
 */
native void Call_PushCell(any value);

/**
 * Pushes a 64-bit integer onto the current call.
 *
 * @note Plugins compiled before SourceMod 1.13 cannot receive a 64-bit integer.
 *       Calling one of them fails with an error. Use Call_PushAddress for an
 *       Address.
 * @note Cannot be used before a call has been started.
 *
 * @param value         64-bit integer value to push.
 * @error               Called before a call has been started.
 */
native void Call_PushInt64(int64 value);

/**
 * Pushes an Address onto the current call.
 *
 * @note Unlike Call_PushInt64, plugins compiled before SourceMod 1.13
 *       receive the low 32 bits of the address.
 * @note Cannot be used before a call has been started.
 *
 * @param value         Address to push.
 * @error               Called before a call has been started.
 */
native void Call_PushAddress(Address value);

/**
 * Pushes a cell by reference onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         Cell reference to push.
 * @error               Called before a call has been started.
 */
native void Call_PushCellRef(any &value);

/**
 * Pushes a float onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         Floating point value to push.
 * @error               Called before a call has been started.
 */
native void Call_PushFloat(float value);

/**
 * Pushes a float by reference onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         Floating point reference to push.
 * @error               Called before a call has been started.
 */
native void Call_PushFloatRef(float &value);

/**
 * Pushes an array onto the current call.
 *
 * @note Changes to array are not copied back to caller. Use PushArrayEx() to do this.
 * @note Cannot be used before a call has been started.
 *
 * @param value         Array to push.
 * @param size          Size of array.
 * @error               Called before a call has been started.
 */
native void Call_PushArray(const any[] value, int size);

/**
 * Pushes an array onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         Array to push.
 * @param size          Size of array.
 * @param cpflags       Whether or not changes should be copied back to the input array.
 *                      See SM_PARAM_* constants for details.
 * @error               Called before a call has been started.
 */
native void Call_PushArrayEx(any[] value, int size, int cpflags);

/**
 * Pushes a string onto the current call.
 *
 * @note Changes to string are not copied back to caller. Use PushStringEx() to do this.
 * @note Cannot be used before a call has been started.
 *
 * @param value         String to push.
 * @error               Called before a call has been started.
 */
native void Call_PushString(const char[] value);

/**
 * Pushes a string onto the current call.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param value         String to push.
 * @param length        Length of string buffer.
 * @param szflags       Flags determining how string should be handled.
 *                      See SM_PARAM_STRING_* constants for details.
 *                      The default (0) is to push ASCII.
 * @param cpflags       Whether or not changes should be copied back to the input array.
 *                      See SM_PARAM_* constants for details.
 * @error               Called before a call has been started.
 */
native void Call_PushStringEx(char[] value, int length, int szflags, int cpflags);


/**
 * Completes a call to a function or forward's call list.
 *
 * @note Cannot be used before a call has been started.
 *
 * @param result        Return value of function or forward's call list.
 * @return              SP_ERROR_NONE on success, any other integer on failure.
 * @error               Called before a call has been started.
 */
native int Call_Finish(any &result=0);

/**
 * Cancels a call to a function or forward's call list.
 *
 * @note Cannot be used before a call has been started.
 *
 * @error               Called before a call has been started.
 */
native void Call_Cancel();

typeset NativeCall
{
	/**
	 * Defines a native function.
	 *
	 * It is not necessary to validate the parameter count
	 *
	 * @param plugin        Handle of the calling plugin.
	 * @param numParams     Number of parameters passed to the native.
	 * @return              Value for the native call to return.
	 */
	function int (Handle plugin, int numParams);

	/**
	 * Defines a native function.
	 *
	 * It is not necessary to validate the parameter count
	 *
	 * @param plugin         Handle of the calling plugin.
	 * @param numParams      Number of parameters passed to the native.
	 * @return               Value for the native call to return.
	 */
	function any (Handle plugin, int numParams);

	/**
	 * Defines a native function.
	 *
	 * It is not necessary to validate the parameter count
	 *
	 * @param plugin         Handle of the calling plugin.
	 * @param numParams      Number of parameters passed to the native.
	 */
	function void (Handle plugin, int numParams);
}

/**
 * Flags for dynamic natives.
 */
enum NativeFlags
{
	NativeFlag_None           = 0,
	NativeFlag_ReturnsInt64   = (1<<0),  /**< The native is declared as returning a 64-bit integer */
	NativeFlag_ReturnsAddress = (1<<1)   /**< The native is declared as returning an Address */
};

/**
 * Creates a dynamic native.  This should only be called in AskPluginLoad(), or
 * else you risk not having your native shared with other plugins.
 *
 * @note A native declared as returning an Address must be created with
 *       NativeFlag_ReturnsAddress, and one returning any other int64 with
 *       NativeFlag_ReturnsInt64. Its return value can be set with
 *       SetNativeReturnAddress or SetNativeReturnInt64, or returned as a cell,
 *       which is zero-extended. Plugins compiled before SourceMod 1.13 receive
 *       the low 32 bits of an Address, and cannot call a native returning any
 *       other int64.
 *
 * @param name          Name of the dynamic native; must be unique among
 *                      all other registered dynamic natives.
 * @param func          Function to use as the dynamic native.
 * @param flags         Flags for the native.
 */
native void CreateNative(const char[] name, NativeCall func, NativeFlags flags = NativeFlag_None);

/**
 * Throws an error in the calling plugin of a native, instead of your own plugin.
 *
 * @param error         Error code to use.
 * @param fmt           Error message format.
 * @param ...           Format arguments.
 * @noreturn
 * @error               Always!
 */
native int ThrowNativeError(int error, const char[] fmt, any ...);

/**
 * Retrieves the string length from a native parameter string.  This is useful for
 * fetching the entire string using dynamic arrays.
 *
 * @note If this function succeeds, Get/SetNativeString will also succeed.
 *
 * @param param         Parameter number, starting from 1.
 * @param length        Stores the length of the string.
 * @return              SP_ERROR_NONE on success, any other integer on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int GetNativeStringLength(int param, int &length);

/**
 * Retrieves a string from a native parameter.
 *
 * @note Output conditions are undefined on failure.
 *
 * @param param         Parameter number, starting from 1.
 * @param buffer        Buffer to store the string in.
 * @param maxlength     Maximum length of the buffer.
 * @param bytes         Optionally store the number of bytes written.
 * @return              SP_ERROR_NONE on success, any other integer on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int GetNativeString(int param, char[] buffer, int maxlength, int &bytes=0);

/**
 * Sets a string in a native parameter.
 *
 * @note Output conditions are undefined on failure.
 *
 * @param param         Parameter number, starting from 1.
 * @param source        Source string to use.
 * @param maxlength     Maximum number of bytes to write.
 * @param utf8          If false, string will not be written
 *                      with UTF8 safety.
 * @param bytes         Optionally store the number of bytes written.
 * @return              SP_ERROR_NONE on success, any other integer on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int SetNativeString(int param, const char[] source, int maxlength, bool utf8=true, int &bytes=0);

/**
 * Gets a cell from a native parameter.
 *
 * @note For an int64 parameter, such as an Address, this returns a reference
 *       to the value. Use GetNativeAddress or GetNativeInt64 to read the value
 *       instead.
 *
 * @param param         Parameter number, starting from 1.
 * @return              Cell value at the parameter number.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native any GetNativeCell(int param);

/**
 * Gets a 64-bit integer from a native parameter.
 *
 * @note Plugins compiled before SourceMod 1.13 cannot pass a 64-bit integer.
 *       Calling this for one of them fails with an error.
 *       Use GetNativeAddress for an Address.
 *
 * @param param         Parameter number, starting from 1.
 * @return              64-bit integer value at the parameter number.
 * @error               Invalid parameter number, calling from a non-native function,
 *                      or a caller compiled before SourceMod 1.13.
 */
native int64 GetNativeInt64(int param);

/**
 * Gets an Address from a native parameter.
 *
 * @note Callers compiled before SourceMod 1.13 pass a single cell, which is
 *       zero-extended.
 *
 * @param param         Parameter number, starting from 1.
 * @return              Address at the parameter number.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native Address GetNativeAddress(int param);

/**
 * Sets a 64-bit integer from a native parameter, by reference.
 *
 * @note Plugins compiled before SourceMod 1.13 cannot receive a 64-bit
 *       integer. Calling this for one of them fails with an error.
 *       Use SetNativeAddressRef for an Address.
 *
 * @param param         Parameter number, starting from 1.
 * @param value         64-bit integer value at the parameter number to set by reference.
 * @error               Invalid parameter number, calling from a non-native function,
 *                      or a caller compiled before SourceMod 1.13.
 */
native void SetNativeInt64Ref(int param, int64 value);

/**
 * Sets an Address from a native parameter, by reference.
 *
 * @note Callers compiled before SourceMod 1.13 receive the low 32 bits.
 *
 * @param param         Parameter number, starting from 1.
 * @param value         Address at the parameter number to set by reference.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native void SetNativeAddressRef(int param, Address value);

/**
 * Sets the return value of a native created with NativeFlag_ReturnsInt64.
 * This value is returned instead of the native function's return value.
 *
 * @param value         64-bit integer value to return.
 * @return              0, so a native can return SetNativeReturnInt64(value).
 * @error               Calling from a non-native function, or the native was not
 *                      created with NativeFlag_ReturnsInt64.
 */
native any SetNativeReturnInt64(int64 value);

/**
 * Sets the return value of a native created with NativeFlag_ReturnsAddress.
 * This value is returned instead of the native function's return value.
 *
 * @note Callers compiled before SourceMod 1.13 receive the low 32 bits.
 *
 * @param value         Address to return.
 * @return              0, so a native can return SetNativeReturnAddress(value).
 * @error               Calling from a non-native function, or the native was not
 *                      created with NativeFlag_ReturnsAddress.
 */
native any SetNativeReturnAddress(Address value);

/**
 * Gets a function pointer from a native parameter.
 *
 * @param param             Parameter number, starting from 1.
 * @return                  Function pointer at the given parameter number.
 * @error                   Invalid parameter number, or calling from a non-native function.
 */
native Function GetNativeFunction(int param);

/**
 * Gets a cell from a native parameter, by reference.
 *
 * @param param         Parameter number, starting from 1.
 * @return              Cell value at the parameter number.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native any GetNativeCellRef(int param);

/**
 * Sets a cell from a native parameter, by reference.
 *
 * @param param         Parameter number, starting from 1.
 * @param value         Cell value at the parameter number to set by reference.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native void SetNativeCellRef(int param, any value);

/**
 * Gets an array from a native parameter (always by reference).
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy into.
 * @param size          Maximum size of local array.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int GetNativeArray(int param, any[] local, int size);

/**
 * Copies a local array into a native parameter array (always by reference).
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy from.
 * @param size          Size of the local array to copy.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int SetNativeArray(int param, const any[] local, int size);

/**
 * Gets an array of 64-bit integers from a native parameter (always by reference).
 *
 * @note Plugins compiled before SourceMod 1.13 cannot pass a 64-bit integer.
 *       Calling this for one of them fails with an error.
 *       Use GetNativeAddressArray for Addresses.
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy into.
 * @param size          Maximum size of local array.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number, calling from a non-native function,
 *                      or a caller compiled before SourceMod 1.13.
 */
native int GetNativeInt64Array(int param, int64[] local, int size);

/**
 * Gets an array of Addresses from a native parameter (always by reference).
 *
 * @note Callers compiled before SourceMod 1.13 pass an array of single cells,
 *       which are zero-extended.
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy into.
 * @param size          Maximum size of local array.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int GetNativeAddressArray(int param, Address[] local, int size);

/**
 * Copies a local array of 64-bit integers into a native parameter array (always by reference).
 *
 * @note Plugins compiled before SourceMod 1.13 cannot receive a 64-bit
 *       integer. Calling this for one of them fails with an error.
 *       Use SetNativeAddressArray for Addresses.
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy from.
 * @param size          Size of the local array to copy.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number, calling from a non-native function,
 *                      or a caller compiled before SourceMod 1.13.
 */
native int SetNativeInt64Array(int param, const int64[] local, int size);

/**
 * Copies a local array of Addresses into a native parameter array (always by reference).
 *
 * @note Callers compiled before SourceMod 1.13 receive the low 32 bits of each
 *       value.
 *
 * @param param         Parameter number, starting from 1.
 * @param local         Local array to copy from.
 * @param size          Size of the local array to copy.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 * @error               Invalid parameter number or calling from a non-native function.
 */
native int SetNativeAddressArray(int param, const Address[] local, int size);

/**
 * Check if the native parameter is the NULL_VECTOR.
 *
 * @param param         Parameter number, starting from 1.
 * @return              True if NULL_VECTOR, false otherwise.
 */
native bool IsNativeParamNullVector(int param);

/**
 * Check if the native parameter is the NULL_STRING.
 *
 * @param param         Parameter number, starting from 1.
 * @return              True if NULL_STRING, false otherwise.
 */
native bool IsNativeParamNullString(int param);

/**
 * Formats a string using parameters from a native.
 *
 * @note All parameter indexes start at 1.
 * @note If the input and output buffers overlap, the contents
 *       of the output buffer at the end is undefined.
 *
 * @param out_param     Output parameter number to write to.  If 0, out_string is used.
 * @param fmt_param     Format parameter number.  If 0, fmt_string is used.
 * @param vararg_param  First variable parameter number.
 * @param out_len       Output string buffer maximum length (always required).
 * @param written       Optionally stores the number of bytes written.
 * @param out_string    Output string buffer to use if out_param is not used.
 * @param fmt_string    Format string to use if fmt_param is not used.
 * @return              SP_ERROR_NONE on success, anything else on failure.
 */
native int FormatNativeString(int out_param,
						  int fmt_param,
						  int vararg_param,
						  int out_len,
						  int &written=0,
						  char[] out_string="",
						  const char[] fmt_string="");

/**
 * Defines a RequestFrame Callback.
 *
 * @param data          Data passed to the RequestFrame native.
 */
typeset RequestFrameCallback {
	function void ();
	function void (any data);
}

/**
 * Creates a single use Next Frame hook.
 *
 * @param Function      Function to call on the next frame.
 * @param data          Value to be passed on the invocation of the Function.
 */
native void RequestFrame(RequestFrameCallback Function, any data=0);
