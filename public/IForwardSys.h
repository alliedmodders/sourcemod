/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#ifndef _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
#define _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_

/**
 * @file IForwardSys.h
 * @brief Defines the interface for managing collections ("forwards") of plugin calls.
 *
 *   The Forward System is responsible for managing automated collections of IPluginFunctions.  
 * It thus provides wrappers to calling many functions at once.  There are two types of such 
 * wrappers: Managed and Unmanaged.  Confusingly, these terms refer to whether the user manages
 * the forwards, not Core.  Managed forwards are completely managed by the user, and are custom
 * editable collections.  Unmanaged forwards are the opposite, and will only work on a single global
 * function name in all plugins.
 */

#include <IPluginSys.h>
#include <sp_vm_api.h>

using namespace SourcePawn;

#define SMINTERFACE_FORWARDMANAGER_NAME		"IForwardManager"
#define SMINTERFACE_FORWARDMANAGER_VERSION	4

/*
 * There is some very important documentation at the bottom of this file.
 * Readers interested in knowing more about the forward system, scrolling down is a must!
 */

namespace SourceMod
{
	/**
	 * @brief Defines the event hook result types plugins can return.
	 */
	enum ResultType
	{
		Pl_Continue = 0,	/**< No result */
		Pl_Changed = 1,		/**< Inputs or outputs have been overridden with new values */
		Pl_Handled = 3,		/**< Result was handled, stop at the end */
		Pl_Stop = 4,		/**< Result was handled, stop now */
	};

	/**
	 * @brief Defines how a forward iterates through plugin functions.
	 */
	enum ExecType
	{
		ET_Ignore = 0,		/**< Ignore all return values, return 0 */
		ET_Single = 1,		/**< Only return the last exec, ignore all others */
		ET_Event = 2,		/**< Acts as an event with the ResultTypes above, no mid-Stops allowed, returns highest */
		ET_Hook = 3,		/**< Acts as a hook with the ResultTypes above, mid-Stops allowed, returns highest */
		ET_LowEvent = 4,	/**< Same as ET_Event except that it returns the lowest value */
	};
	
	#define SP_PARAMTYPE_ANY	0
	#define SP_PARAMFLAG_BYREF	(1<<0)
	#define SP_PARAMTYPE_CELL	(1<<1)
	#define SP_PARAMTYPE_FLOAT	(2<<1)
	#define SP_PARAMTYPE_STRING	(3<<1)|SP_PARAMFLAG_BYREF
	#define SP_PARAMTYPE_ARRAY	(4<<1)|SP_PARAMFLAG_BYREF
	#define SP_PARAMTYPE_VARARG	(5<<1)

	/**
	 * @brief Describes the various ways to pass parameters to plugins.
	 */
	enum ParamType
	{
		Param_Any = SP_PARAMTYPE_ANY,				/**< Any data type can be pushed */
		Param_Cell = SP_PARAMTYPE_CELL,				/**< Only basic cells can be pushed */
		Param_Float = SP_PARAMTYPE_FLOAT,			/**< Only floats can be pushed */
		Param_String = SP_PARAMTYPE_STRING,			/**< Only strings can be pushed */
		Param_Array = SP_PARAMTYPE_ARRAY,			/**< Only arrays can be pushed */
		Param_VarArgs = SP_PARAMTYPE_VARARG,		/**< Same as "..." in plugins, anything can be pushed, but it will always be byref */
		Param_CellByRef = SP_PARAMTYPE_CELL|SP_PARAMFLAG_BYREF,		/**< Only a cell by reference can be pushed */
		Param_FloatByRef = SP_PARAMTYPE_FLOAT|SP_PARAMFLAG_BYREF,	/**< Only a float by reference can be pushed */
	};
	
	struct ByrefInfo
	{
		unsigned int cells;
		cell_t *orig_addr;
		int flags;
		int sz_flags;
	};

	struct FwdParamInfo
	{
		cell_t val;
		ByrefInfo byref;
		ParamType pushedas;
		bool isnull;
	};
	
	class IForwardFilter
	{
	public:
		virtual void Preprocess(IPluginFunction *fun, FwdParamInfo *params)
		{
		}
	};

	/**
	 * @brief Unmanaged Forward, abstracts calling multiple functions as "forwards," or collections of functions.
	 * 
	 *  Parameters should be pushed in forward order, unlike the virtual machine/IPluginContext order.
	 * Some functions are repeated in here because their documentation differs from their IPluginFunction equivalents.
	 * Missing are the Push functions, whose only doc change is that they throw SP_ERROR_PARAM on type mismatches.
	 */
	class IForward : public ICallable
	{
	public:
		/** Virtual Destructor */
		virtual ~IForward()
		{
		}
	public:
		/**
		 * @brief Returns the name of the forward.
		 *
		 * @return			Forward name.
		 */
		virtual const char *GetForwardName() =0;

		/**
		 * @brief Returns the number of functions in this forward.
		 *
		 * @return			Number of functions in forward.
		 */
		virtual unsigned int GetFunctionCount() =0;

		/**
		 * @brief Returns the method of multi-calling this forward has.
		 *
		 * @return			ExecType of the forward.
		 */
		virtual ExecType GetExecType() =0;

		/**
		 * @brief Executes the forward.
		 *
		 * @param result		Optional pointer to store result in.
		 * @param filter		Do not use.
		 * @return				Error code, if any.
		 */
		virtual int Execute(cell_t *result=NULL, IForwardFilter *filter=NULL) =0;

		/**
		 * @brief Pushes an array of cells onto the current call.  Different rules than ICallable.
		 * NOTE: On Execute, the pointer passed will be modified according to the copyback rule.
		 *
		 * @param inarray	Array to copy.  If NULL and cells is 3 pushes a reference to the NULL_VECTOR pubvar to each callee.
		 *                  Pushing other number of cells is not allowed, unlike ICallable's version.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param flags		Whether or not changes should be copied back to the input array.
		 * @return			Error code, if any.
		 */
		virtual int PushArray(cell_t *inarray, unsigned int cells, int flags=0) =0;

		/**
		* @brief Pushes a string onto the current call.
		*
		* @param string  String to push.  If NULL pushes a reference to the NULL_STRING pubvar to each callee.
		* @return      Error code, if any.
		*/
		virtual int PushString(const char *string) = 0;
	};

	/**
	 * @brief Managed Forward, same as IForward, except the collection can be modified.
	 */
	class IChangeableForward : public IForward
	{
	public:
		/** 
		 * @brief Removes a function from the call list.
		 * NOTE: Only removes one instance.
		 *
		 * @param func		Function to remove.
		 * @return			Whether or not the function was removed.
		 */
		virtual bool RemoveFunction(IPluginFunction *func) =0;

		/** 
		 * @brief Removes all instances of a plugin from the call list.
		 *
		 * @param plugin	Plugin to remove instances of.
		 * @return			Number of functions removed therein.
		 */
		virtual unsigned int RemoveFunctionsOfPlugin(IPlugin *plugin) =0;

		/**
		 * @brief Adds a function to the call list.
		 * NOTE: Cannot be used during an incomplete call.
		 * NOTE: If used during a call, function is temporarily queued until calls are over.
		 * NOTE: Adding multiple copies of the same function is illegal.
		 *
		 * @param func		Function to add.
		 * @return			True on success, otherwise false.
		 */
		virtual bool AddFunction(IPluginFunction *func) =0;

		/**
		 * @brief Adds a function to the call list.
		 * NOTE: Cannot be used during an incomplete call.
		 * NOTE: If used during a call, function is temporarily queued until calls are over.
		 *
		 * @param ctx		Context to use as a look-up.
		 * @param index		Function id to add.
		 * @return			True on success, otherwise false.
		 */
		virtual bool AddFunction(IPluginContext *ctx, funcid_t index) =0;

		/** 
		 * @brief Removes a function from the call list.
		 * NOTE: Only removes one instance.
		 *
		 * @param ctx		Context to use as a look-up.
		 * @param index		Function id to add.
		 * @return			Whether or not the function was removed.
		 */
		virtual bool RemoveFunction(IPluginContext *ctx, funcid_t index) =0;
	};
	
	/**
	 * @brief Provides functions for creating/destroying managed and unmanaged forwards.
	 */
	class IForwardManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_FORWARDMANAGER_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_FORWARDMANAGER_VERSION;
		}
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version < 2 || version > GetInterfaceVersion())
			{
				return false;
			}
			return true;
		}
	public:
		/**
		 * @brief Creates a managed forward.  This forward exists globally.
		 *  The name used to create the forward is used as its public function in all target plugins.
		 *  As new non-private plugins become loaded or unloaded, they will be automatically added
		 * or removed.  This is ideal for global, static forwards that are never changed.
		 *
		 * @param name			Name of public function to use in forward.
		 * @param et			Execution type to be used.
		 * @param num_params	Number of parameter this function will have.
		 *						NOTE: For varargs, this should include the vararg parameter.
		 * @param types			Array of type information about each parameter.  If NULL, types
		 *						are read off the vararg stream.
		 * @param ...			If types is NULL, num_params ParamTypes should be pushed.
		 * @return				A new IForward on success, NULL if type combination is impossible.
		 */
		virtual IForward *CreateForward(const char *name, 
										ExecType et, 
										unsigned int num_params, 
										const ParamType *types, 
										...) =0;

		/**
		 * @brief Creates an unmanaged forward.  This forward exists privately.
		 * Unlike managed forwards, no functions are ever added by the Manager.
		 * However, functions will be removed automatically if their parent plugin is unloaded.
		 *
		 * @param name			Name of forward (unused except for lookup, can be NULL for anonymous).
		 * @param et			Execution type to be used.
		 * @param num_params	Number of parameter this function will have.
		 *						NOTE: For varargs, this should include the vararg parameter.
		 * @param types			Array of type information about each parameter.  If NULL, types
		 *						are read off the vararg stream.
		 * @param ...			If types is NULL, num_params ParamTypes should be pushed.
		 * @return				A new IChangeableForward on success, NULL if type combination is impossible.
		 */
		virtual IChangeableForward *CreateForwardEx(const char *name, 
													ExecType et, 
													int num_params, 
													const ParamType *types, 
													...) =0;

		/**
		 * @brief Finds a forward by name.  Does not return anonymous forwards (named NULL or "").
		 *
		 * @param name			Name of forward.
		 * @param ifchng		Optionally store either NULL or an IChangeableForward pointer
		 *                      depending on type of forward.
		 * @return				IForward pointer, or NULL if none found matching the name.
		 */
		virtual IForward *FindForward(const char *name, IChangeableForward **ifchng) =0;

		/**
		 * @brief Frees and destroys a forward object.
		 *
		 * @param forward		An IForward created by CreateForward() or CreateForwardEx().
		 */
		virtual void ReleaseForward(IForward *forward) =0;
	};
}

/*
 *  In the AMX Mod X model of forwarding, each forward contained a list of pairs, each pair containing
 * a function ID and an AMX structure.  The forward structure itself did very little but hold parameter types.
 * An execution call worked like this:
 *  - executeForward() took in a function id and a list of parameters
 *  - for each contained plugin:
 *   - the list of parameters was preprocessed and pushed
 *   - the call was made
 *   - the list was freed and copybacks were made
 *  - return
 * 
 *  The advantages to this is that the system is very easy to implement, and it's fast. The disadvantage is 
 * varargs tend to be very unforgiving and inflexible, and thus weird problems arose with casting.  You also 
 * lose flexibility, type checking, and the ability to reasonably use variable arguments lists in the VM.
 *
 *  SourceMod replaces this forward system with a far more advanced, but a bit bulkier one. The idea is that 
 * each plugin has a table of functions, and each function is an ICallable object.  As well as being an ICallable, 
 * each function is an IPluginFunction.  An ICallable simply describes the process of adding parameters to a 
 * function call.  An IPluginFunction describes the process of actually calling a function and performing allocation, 
 * copybacks, and deallocations.
 *
 * A very powerful forward system emerges: a Forward is just a collection of IPluginFunctions.  Thus, the same 
 * API can be easily wrapped around a simple list, and it will look transparent to the user.
 * Advantages:
 * 1) "SP Forwards" from AMX Mod X are simply IPluginFunctions without a collection.
 * 2) Forwards are function based, rather than plugin based, and are thus far more flexible at runtime..
 * 3) [2] Individual functions can be paused and more than one function from the same plugin can be hooked.
 * 4) [2] One hook type that used to map to many SP Forwards can now be centralized as one Forward.
 *    This helps alleviate messes like Fakemeta.
 * 5) Parameter pushing is type-checked and allows for variable arguments.
 *
 *  Note that while #2,3,4 could be added to AMX Mod X, the real binding property is #1, which makes the system
 * object oriented, rather than AMX Mod X, which hides the objects behind static functions.  It is entirely a design
 * issue, rather than a usability one.  The interesting part is when it gets to implementation, which has to cache
 * parameter pushing until execution.  Without this, multiple function calls can be started across one plugin, which
 * will result in heap corruption given SourcePawn's implementation.
 *
 * Observe the new calling process:
 * - Each parameter is pushed into a local cache using the ICallable interface.
 * - For each function in the collection:
 *  - Each parameter is decoded and -pushed into the function.
 *  - The call is made.
 * - Return
 *
 *  Astute readers will note the (minor) problems:
 * 1) More memory is used.  Specifically, rather than N params of memory, you now have N params * M plugins.
 *    This is because, again, parameters are cached both per-function and per-forward.
 * 2) There are slightly more calls going around: one extra call for each parameter, since each push is manual.
 *
 * HISTORICAL NOTES:
 *  There used to be a # about copy backs.  
 *  Note that originally, the Forward implementation was a thin wrapper around IForwards.  It did not cache pushes,
 * and instead immediately fired them to each internal plugin.  This was to allow users to know that pointers would 
 * be immediately resolved.  Unfortunately, this became extremely burdensome on the API and exposed many problems,
 * the major (and breaking) one was that two separate Function objects cannot be in a calling process on the same 
 * plugin at once. (:TODO: perhaps prevent that in the IPlugin object?) This is because heap functions lose their order
 * and become impossible to re-arrange without some global heap tracking mechanism.  It also made iterative copy backs 
 * for arrays/references overwhelmingly complex, since each plugin had to have its memory back-patched for each copy.
 *  Therefore, this was scrapped for cached parameters (current implementation), which is the implementation AMX Mod X 
 * uses.  It is both faster and works better.
 */

#endif //_INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
