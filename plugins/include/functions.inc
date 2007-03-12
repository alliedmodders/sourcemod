/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */
 
#define SP_ERROR_NONE					0	/**< No error occurred */
#define SP_ERROR_FILE_FORMAT			1	/**< File format unrecognized */
#define SP_ERROR_DECOMPRESSOR			2	/**< A decompressor was not found */
#define SP_ERROR_HEAPLOW				3	/**< Not enough space left on the heap */
#define SP_ERROR_PARAM					4	/**< Invalid parameter or parameter type */
#define SP_ERROR_INVALID_ADDRESS		5	/**< A memory address was not valid */
#define SP_ERROR_NOT_FOUND				6	/**< The object in question was not found */
#define SP_ERROR_INDEX					7	/**< Invalid index parameter */
#define SP_ERROR_STACKLOW				8	/**< Nnot enough space left on the stack */
#define SP_ERROR_NOTDEBUGGING			9	/**< Debug mode was not on or debug section not found */
#define SP_ERROR_INVALID_INSTRUCTION	10	/**< Invalid instruction was encountered */
#define SP_ERROR_MEMACCESS				11	/**< Invalid memory access */
#define SP_ERROR_STACKMIN				12	/**< Stack went beyond its minimum value */
#define SP_ERROR_HEAPMIN				13  /**< Heap went beyond its minimum value */
#define SP_ERROR_DIVIDE_BY_ZERO			14	/**< Division by zero */
#define SP_ERROR_ARRAY_BOUNDS			15	/**< Array index is out of bounds */
#define SP_ERROR_INSTRUCTION_PARAM		16	/**< Instruction had an invalid parameter */
#define SP_ERROR_STACKLEAK				17  /**< A native leaked an item on the stack */
#define SP_ERROR_HEAPLEAK				18  /**< A native leaked an item on the heap */
#define SP_ERROR_ARRAY_TOO_BIG			19	/**< A dynamic array is too big */
#define SP_ERROR_TRACKER_BOUNDS			20	/**< Tracker stack is out of bounds */
#define SP_ERROR_INVALID_NATIVE			21	/**< Native was pending or invalid */
#define SP_ERROR_PARAMS_MAX				22	/**< Maximum number of parameters reached */
#define SP_ERROR_NATIVE					23	/**< Error originates from a native */
#define SP_ERROR_NOT_RUNNABLE			24	/**< Function or plugin is not runnable */
#define SP_ERROR_ABORTED				25	/**< Function call was aborted */

/**
 * Defines a native function.
 *
 * It is not necessary to validate the parameter count 
 *
 * @param plugin			Handle of the calling plugin.
 * @param numParams			Number of parameters passed to the native.
 * @return 					Value for the native call to return.
 */
functag NativeCall public(Handle:plugin, numParams);

/**
 * Creates a dynamic native.  This should only be called in AskPluginLoad(), or 
 * else you risk not having your native shared with other plugins.
 *
 * @param name				Name of the dynamic native; must be unique amongst
 *							all other registered dynamic native.s
 * @param func				Function to use as the dynamic native.
 * @noreturn
 */
native CreateNative(const String:name[], NativeCall:func);

/**
 * Throws an error in the calling plugin of a native, instead of your own plugin.
 *
 * @param error				Error code to use.
 * @param fmt				Error message format.
 * @param ...				Format arguments.
 */
native ThrowNativeError(error, const String:fmt[], {Handle,Float,String,_}:...);

/**
 * Retrieves the string length from a native parameter string.  This is useful 
 * fetching the entire string using dynamic arrays.
 * @note If this function succeeds, Get/SetNativeString will also succeed.
 *
 * @param param				Parameter number, starting from 1.
 * @param length			Stores the length of the string.
 * @return					SP_ERROR_NONE on sucecss, any other integer on failure.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native GetNativeStringLength(param, &length);

/**
 * Retrieves a string from a native parameter.
 * @note Output conditions are undefined on failure.
 *
 * @param param				Parameter number, starting from 1.
 * @param buffer			Buffer to store the string in.
 * @param maxlength			Maximum length of the buffer.
 * @param bytes				Optionally store the number of bytes written.
 * @return					SP_ERROR_NONE on success, any other integer on failure.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native GetNativeString(param, String:buffer[], maxlength, &bytes=0);

/**
 * Sets a string in a native parameter.
 * @note Output conditions are undefined on failure.
 *
 * @param param				Parameter number, starting from 1.
 * @param source			Source string to use.
 * @param maxlength			Maximum number of bytes to write.
 * @param utf8				If false, string will not be written
 *							with UTF8 safety.
 * @param bytes				Optionally store the number of bytes written.
 * @return					SP_ERROR_NONE on success, any other integer on failure.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native SetNativeString(param, const String:source[], maxlength, bool:utf8=true, &bytes=0);

/**
 * Gets a cell from a native parameter.
 *
 * @param param				Parameter number, starting from 1.
 * @return					Cell value at the parameter number.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native GetNativeCell(param);

/**
 * Gets a cell from a native parameter, by reference.
 *
 * @param param				Parameter number, starting from 1.
 * @return					Cell value at the parameter number.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native GetNativeCellRef(param);

/**
 * Sets a cell from a native parameter, by reference.
 *
 * @param param				Parameter number, starting from 1.
 * @param value				Cell value at the parameter number to set by reference.
 * @noreturn
 * @error					Invalid parameter number or calling from a non-native function.
 */
native SetNativeCellRef(param, {Float,Handle,_}:value);

/**
 * Gets an array from a native parameter (always by reference).
 *
 * @param param				Parameter number, starting from 1.
 * @param local				Local array to copy into.
 * @param size				Maximum size of local array.
 * @return					SP_ERROR_NONE on success, anything else on failure.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native GetNativeArray(param, {Float,Handle,_}:local[], size);

/**
 * Copies a local array into a native parameter array (always by reference).
 * 
 * @param param				Parameter number, starting from 1.
 * @param local				Local array to copy from.
 * @param size				Size of the local array to copy.
 * @return					SP_ERROR_NONE on success, anything else on failure.
 * @error					Invalid parameter number or calling from a non-native function.
 */
native SetNativeArray(param, const {Float,Handle,_}:local[], size);

/**
 * Formats a string using parameters from a native.
 *
 * @note All parameter indexes start at 1.
 * @note If the input and output buffers overlap, the contents
 *  of the output buffer at the end is undefined.
 *
 * @param out_param			Output parameter number to write to.  If 0, out_string is used.
 * @param fmt_param			Format parameter number.  If 0, fmt_string is used.
 * @param vararg_param		First variable parameter number.
 * @param out_len			Output string buffer maximum length (always required).
 * @param written			Optionally stores the number of bytes written.
 * @param out_string		Output string buffer to use if out_param is not used.
 * @param fmt_string		Format string to use if fmt_param is not used.
 * @return					SP_ERROR_NONE on success, anything else on failure.
 */
native FormatNativeString(out_param, 
						  fmt_param, 
						  vararg_param, 
						  out_len,
						  &written=0,
						  String:out_string[]="",
						  const String:fmt_string[]="");

stock Float:GetNativeFloat(param, bool:byref)
{
	if (!byref)
	{
		return Float:GetNativeCell(param);
	} else {
		return Float:GetNativeCellRef(param);
	}
}
stock Handle:GetNativeHandle(param, bool:byref)
{
	if (!byref)
	{
		return Handle:GetNativeCell(param);
	} else {
		return Handle:GetNativeCellRef(param);
	}
}
