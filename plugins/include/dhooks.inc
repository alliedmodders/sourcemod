#if defined _dhooks_included
#endinput
#endif
#define _dhooks_included

enum ObjectValueType
{
	ObjectValueType_Int = 0,
	ObjectValueType_Bool,
	ObjectValueType_Ehandle,
	ObjectValueType_Float,
	ObjectValueType_CBaseEntityPtr,
	ObjectValueType_IntPtr,
	ObjectValueType_BoolPtr,
	ObjectValueType_EhandlePtr,
	ObjectValueType_FloatPtr,
	ObjectValueType_Vector,
	ObjectValueType_VectorPtr,
	ObjectValueType_CharPtr,
	ObjectValueType_String
};

enum ListenType
{
	ListenType_Created,
	ListenType_Deleted
};

enum ReturnType
{
	ReturnType_Unknown,
	ReturnType_Void,
	ReturnType_Int,
	ReturnType_Bool,
	ReturnType_Float,
	ReturnType_String, //Note this is a string_t
	ReturnType_StringPtr, //Note this is a string_t *
	ReturnType_CharPtr,
	ReturnType_Vector,
	ReturnType_VectorPtr,
	ReturnType_CBaseEntity,
	ReturnType_Edict
};

enum HookParamType
{
	HookParamType_Unknown,
	HookParamType_Int,
	HookParamType_Bool,
	HookParamType_Float,
	HookParamType_String, //Note this is a string_t
	HookParamType_StringPtr, //Note this is a string_t *
	HookParamType_CharPtr,
	HookParamType_VectorPtr,
	HookParamType_CBaseEntity,
	HookParamType_ObjectPtr,
	HookParamType_Edict,
	HookParamType_Object
};

enum ThisPointerType
{
	ThisPointer_Ignore,
	ThisPointer_CBaseEntity,
	ThisPointer_Address
};

enum HookType
{
	HookType_Entity,
	HookType_GameRules,
	HookType_Raw
};

enum MRESReturn
{
	MRES_ChangedHandled = -2,	// Use changed values and return MRES_Handled
	MRES_ChangedOverride,		// Use changed values and return MRES_Override
	MRES_Ignored,				// plugin didn't take any action
	MRES_Handled,				// plugin did something, but real function should still be called
	MRES_Override,				// call real function, but use my return value
	MRES_Supercede				// skip real function; use my return value
};

enum DHookPassFlag
{
	DHookPass_ByVal = 		(1<<0),		/**< Passing by value */
	DHookPass_ByRef = 		(1<<1),		/**< Passing by reference */
	DHookPass_ODTOR =		(1<<2),		/**< Object has a destructor */
	DHookPass_OCTOR =		(1<<3),		/**< Object has a constructor */
	DHookPass_OASSIGNOP	=	(1<<4),		/**< Object has an assignment operator */
};

typeset ListenCB
{
	//Deleted
	function void (int entity);
	
	//Created
	function void (int entity, const char[] classname);
};

typeset DHookRemovalCB
{
	function void (int hookid);
};
typeset DHookCallback
{
	//Function Example: void Ham::Test() with this pointer ignore
	function MRESReturn ();
	
	//Function Example: void Ham::Test() with this pointer passed
	function MRESReturn (int pThis);
	
	//Function Example: void Ham::Test(int cake) with this pointer ignore
	function MRESReturn (Handle hParams);
	
	//Function Example: void Ham::Test(int cake) with this pointer passed
	function MRESReturn (int pThis, Handle hParams);
	
	//Function Example: int Ham::Test() with this pointer ignore
	function MRESReturn (Handle hReturn);
	
	//Function Example: int Ham::Test() with this pointer passed
	function MRESReturn (int pThis, Handle hReturn);
	
	//Function Example: int Ham::Test(int cake) with this pointer ignore
	function MRESReturn (Handle hReturn, Handle hParams);
	
	//Function Example: int Ham::Test(int cake) with this pointer passed
	function MRESReturn (int pThis, Handle hReturn, Handle hParams);
	
	//Address NOW
	
	//Function Example: void Ham::Test() with this pointer passed
	function MRESReturn (Address pThis);
	
	//Function Example: void Ham::Test(int cake) with this pointer passed
	function MRESReturn (Address pThis, Handle hParams);
	
	//Function Example: int Ham::Test() with this pointer passed
	function MRESReturn (Address pThis, Handle hReturn);
	
	//Function Example: int Ham::Test(int cake) with this pointer passed
	function MRESReturn (Address pThis, Handle hReturn, Handle hParams);
	
};

/* Adds an entity listener hook
 *
 * @param type			Type of listener to add
 * @param callback		Callback to use
 *
 * @noreturn
*/
native void DHookAddEntityListener(ListenType type, ListenCB callback);

/* Removes an entity listener hook
 *
 * @param type			Type of listener to remove
 * @param callback		Callback this listener was using
 *
 * @return True if one was removed false otherwise.
*/
native bool DHookRemoveEntityListener(ListenType type, ListenCB callback);

/* Creates a hook
 *
 * @param offset		vtable offset for function to hook
 * @param hooktype		Type of hook
 * @param returntype	Type type of return
 * @param thistype		Type of this pointer or ignore (ignore can be used if not needed)
 * @param callback		Optional callback function, if not set here must be set when hooking.
 * 
 * @return Returns setup handle for the hook or INVALID_HANDLE.
*/
native Handle DHookCreate(int offset, HookType hooktype, ReturnType returntype, ThisPointerType thistype, DHookCallback callback=INVALID_FUNCTION);

/* Adds param to a hook setup
 *
 * @param setup			Setup handle to add the param to.
 * @param type			Param type
 * @param size			Used for Objects (not Object ptr) to define the size of the object.
 * @param flag			Used to change the pass type.
 * 
 * @error	Invalid setup handle or too many params added (request upping the max in thread)
 * @noreturn
*/
native void DHookAddParam(Handle setup, HookParamType type, int size=-1, DHookPassFlag flag=DHookPass_ByVal);

/* Hook entity
 * 
 * @param setup			Setup handle to use to add the hook.
 * @param post			True to make the hook a post hook. (If you need to change the retunr value or need the return value use a post hook! If you need to change params and return use a pre and post hook!)
 * @param entity		Entity index to hook on.
 * @param removalcb		Callback for when the hook is removed (Entity hooks are auto-removed on entity destroyed and will call this callback)
 * @param callback		Optional callback function, if not set here must be set when creating the hook.
 * 
 * @error Invalid setup handle, invalid address, invalid hook type or invalid callback.
 * @return -1 on fail a hookid on success
*/
native int DHookEntity(Handle setup, bool post, int entity, DHookRemovalCB removalcb=INVALID_FUNCTION, DHookCallback callback=INVALID_FUNCTION);

/* Hook gamerules
 * 
 * @param setup			Setup handle to use to add the hook.
 * @param post			True to make the hook a post hook. (If you need to change the retunr value or need the return value use a post hook! If you need to change params and return use a pre and post hook!)
 * @param removalcb		Callback for when the hook is removed (Game rules hooks are auto-removed on map end and will call this callback)
 * @param callback		Optional callback function, if not set here must be set when creating the hook.
 * 
 * @error Invalid setup handle, invalid address, invalid hook type or invalid callback.
 * @return -1 on fail a hookid on success
*/
native int DHookGamerules(Handle setup, bool post, DHookRemovalCB removalcb=INVALID_FUNCTION, DHookCallback callback=INVALID_FUNCTION);

/* Hook a raw pointer
 * 
 * @param setup			Setup handle to use to add the hook.
 * @param post			True to make the hook a post hook. (If you need to change the retunr value or need the return value use a post hook! If you need to change params and return use a pre and post hook!)
 * @param addr			This pointer address.
 * @param removalcb		Callback for when the hook is removed (Entity hooks are auto-removed on entity destroyed and will call this callback)
 * @param callback		Optional callback function, if not set here must be set when creating the hook.
 * 
 * @error Invalid setup handle, invalid address, invalid hook type or invalid callback.
 * @return -1 on fail a hookid on success
*/
native int DHookRaw(Handle setup, bool post, Address addr, DHookRemovalCB removalcb=INVALID_FUNCTION, DHookCallback callback=INVALID_FUNCTION);

/* Remove hook by hook id
 * 
 * @param hookid		Hook id to remove
 * 
 * @return true on success false otherwise
 * @note This will not fire the removal callback!
*/
native bool DHookRemoveHookID(int hookid);

/* Get param value (Only use for: int, entity, bool or float param types)
 * 
 * @param hParams		Handle to params structure
 * @param num			Param number to get. (Example if the function has 2 params and you need the value of the first param num would be 1. 0 Will return the number of params stored)
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @return value if num greater than 0. If 0 returns paramcount.
*/
native any DHookGetParam(Handle hParams, int num);

/* Get vector param value
 * 
 * @param hParams		Handle to params structure
 * @param num			Param number to get. (Example if the function has 2 params and you need the value of the first param num would be 1.)
 * @param vec			Vector buffer to store result.
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @noreturn
*/
native void DHookGetParamVector(Handle hParams, int num, float vec[3]);

/* Get string param value
 * 
 * @param hParams		Handle to params structure
 * @param num			Param number to get. (Example if the function has 2 params and you need the value of the first param num would be 1.)
 * @param buffer		String buffer to store result
 * @param size			Buffer size
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @noreturn
*/
native void DHookGetParamString(Handle hParams, int num, char[] buffer, int size);

/* Set param value (Only use for: int, entity, bool or float param types)
 * 
 * @param hParams		Handle to params structure
 * @params num			Param number to set (Example if the function has 2 params and you need to set the value of the first param num would be 1.)
 * @param value			Value to set it as (only pass int, bool, float or entity index)
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @noreturn
*/
native void DHookSetParam(Handle hParams, int num, any value);

/* Set vector param value
 * 
 * @param hParams		Handle to params structure
 * @params num			Param number to set (Example if the function has 2 params and you need to set the value of the first param num would be 1.)
 * @param vec			Value to set vector as.
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @noreturn
*/
native void DHookSetParamVector(Handle hParams, int num, float vec[3]);

/* Set string param value
 * 
 * @param hParams		Handle to params structure
 * @params num			Param number to set (Example if the function has 2 params and you need to set the value of the first param num would be 1.)
 * @param value			Value to set string as.
 * 
 * @error Invalid handle. Invalid param number. Invalid param type.
 * @noreturn
*/
native void DHookSetParamString(Handle hParams, int num, char[] value);

/* Get return value (Only use for: int, entity, bool or float return types)
 * 
 * @param hReturn		Handle to return structure
 * 
 * @error Invalid Handle, invalid type.
 * @return Returns default value if prehook returns actual value if post hook.
*/
native any DHookGetReturn(Handle hReturn);

/* Get return vector value
 * 
 * @param hReturn		Handle to return structure
 * @param vec			Vector buffer to store result in. (In pre hooks will be default value (0.0,0.0,0.0))
 * 
 * @error Invalid Handle, invalid type.
 * @noreturn
*/
native void DHookGetReturnVector(Handle hReturn, float vec[3]);

/* Get return string value
 * 
 * @param hReturn		Handle to return structure
 * @param buffer		String buffer to store result in. (In pre hooks will be default value "")
 * @param size			String buffer size
 * 
 * @error Invalid Handle, invalid type.
 * @noreturn
*/
native void DHookGetReturnString(Handle hReturn, char[] buffer, int size);

/* Set return value (Only use for: int, entity, bool or float return types)
 * 
 * @param hReturn		Handle to return structure
 * @param value			Value to set return as
 * 
 * @error Invalid Handle, invalid type.
 * @noreturn
*/
native void DHookSetReturn(Handle hReturn, any value);

/* Set return vector value
 * 
 * @param hReturn		Handle to return structure
 * @param vec			Value to set return vector as
 * 
 * @error Invalid Handle, invalid type.
 * @noreturn
*/
native void DHookSetReturnVector(Handle hReturn, float vec[3]);

/* Set return string value
 * 
 * @param hReturn		Handle to return structure
 * @param value			Value to set return string as
 * 
 * @error Invalid Handle, invalid type.
 * @noreturn
*/
native void DHookSetReturnString(Handle hReturn, char[] value);

//WE SHOULD WRAP THESE AROUND STOCKS FOR NON PTR AS WE SUPPORT BOTH WITH THESE NATIVE'S

/* Gets an objects variable value
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to get.
 * @param offset		Offset within the object to the var to get.
 * @param type			Type of var it is
 *
 * @error Invalid handle. Invalid param number. Invalid param type. Invalid Object type.
 * @return Value of the objects var. If EHANDLE type or entity returns entity index.
*/
native any DHookGetParamObjectPtrVar(Handle hParams, int num, int offset, ObjectValueType type);

/* Sets an objects variable value
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to set.
 * @param offset		Offset within the object to the var to set.
 * @param type			Type of var it is
 * @param value			The value to set the var to.
 *
 * @error Invalid handle. Invalid param number. Invalid param type. Invalid Object type.
 * @noreturn
*/
native void DHookSetParamObjectPtrVar(Handle hParams, int num, int offset, ObjectValueType type, any value);

/* Gets an objects vector variable value
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to get.
 * @param offset		Offset within the object to the var to get.
 * @param type			Type of var it is
 * @param buffer		Buffer to store the result vector
 *
 * @error Invalid handle. Invalid param number. Invalid param type. Invalid Object type.
 * @noreturn
*/
native void DHookGetParamObjectPtrVarVector(Handle hParams, int num, int offset, ObjectValueType type, float buffer[3]);

/* Sets an objects vector variable value
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to set.
 * @param offset		Offset within the object to the var to set.
 * @param type			Type of var it is
 * @param value			The value to set the vector var to.
 *
 * @error Invalid handle. Invalid param number. Invalid param type. Invalid Object type.
 * @noreturn
*/
native void DHookSetParamObjectPtrVarVector(Handle hParams, int num, int offset, ObjectValueType type, float value[3]);

/* Gets an objects string variable value
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to get.
 * @param offset		Offset within the object to the var to get.
 * @param type			Type of var it is
 * @param buffer		Buffer to store the result vector
 * @param size			Size of the buffer
 *
 * @error Invalid handle. Invalid param number. Invalid param type. Invalid Object type.
 * @noreturn
*/
native void DHookGetParamObjectPtrString(Handle hParams, int num, int offset, ObjectValueType type, char[] buffer, int size);

/* Checks if a pointer param is null
 *
 * @param hParams		Handle to params structure
 * @param num			Param number to check.
 *
 * @error Non pointer param
 * @return True if null false otherwise.
*/
native bool DHookIsNullParam(Handle hParams, int num);

public Extension __ext_dhooks =
{
	name = "dhooks",
	file = "dhooks.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_dhooks_SetNTVOptional()
{
	MarkNativeAsOptional("DHookAddEntityListener");
	MarkNativeAsOptional("DHookRemoveEntityListener");
	MarkNativeAsOptional("DHookCreate");
	MarkNativeAsOptional("DHookAddParam");
	MarkNativeAsOptional("DHookEntity");
	MarkNativeAsOptional("DHookGamerules");
	MarkNativeAsOptional("DHookRaw");
	MarkNativeAsOptional("DHookRemoveHookID");
	MarkNativeAsOptional("DHookGetParam");
	MarkNativeAsOptional("DHookGetParamVector");
	MarkNativeAsOptional("DHookGetParamString");
	MarkNativeAsOptional("DHookSetParam");
	MarkNativeAsOptional("DHookSetParamVector");
	MarkNativeAsOptional("DHookSetParamString");
	MarkNativeAsOptional("DHookGetReturn");
	MarkNativeAsOptional("DHookGetReturnVector");
	MarkNativeAsOptional("DHookGetReturnString");
	MarkNativeAsOptional("DHookSetReturn");
	MarkNativeAsOptional("DHookSetReturnVector");
	MarkNativeAsOptional("DHookSetReturnString");
	MarkNativeAsOptional("DHookGetParamObjectPtrVar");
	MarkNativeAsOptional("DHookSetParamObjectPtrVar");
	MarkNativeAsOptional("DHookGetParamObjectPtrVarVector");
	MarkNativeAsOptional("DHookSetParamObjectPtrVarVector");
	MarkNativeAsOptional("DHookIsNullParam");
	MarkNativeAsOptional("DHookGetParamObjectPtrString");
}
#endif