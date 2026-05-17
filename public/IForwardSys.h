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
 * A Forward is just a collection of IPluginFunctions.  Thus, the same API can
 * be easily wrapped around a simple list, and it will look transparent to the
 * user.
 *
 * Forwards are function based, rather than plugin based, and are thus far more flexible at runtime..
 * Individual functions can be paused and more than one function from the same plugin can be hooked.
 * Parameter pushing is type-checked and allows for variable arguments.
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
		virtual void Preprocess(IPluginFunction *fun, sp::CallArgs& args)
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
	class IForward
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

		/**
		 * @brief Pushes a cell onto the current call.
		 *
		 * @param cell	Parameter value to push.
		 * @return	  Error code, if any.
		 */
		virtual int PushCell(cell_t cell) = 0;
	
		/**
		 * @brief Pushes a cell by reference onto the current call.
		 * NOTE: On Execute, the pointer passed will be modified if copyback is enabled.
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *	 This means you cannot push a pointer, change it, and push it again and expect
		 *	   two different values to come out.
		 *
		 * @param cell	Address containing parameter value to push.
		 * @param flags	Copy-back flags.
		 * @return	  Error code, if any.
		 */
		virtual int PushCellByRef(cell_t* cell, int flags = SM_PARAM_COPYBACK) = 0;
	
		/**
		 * @brief Pushes a float onto the current call.
		 *
		 * @param number  Parameter value to push.
		 * @return	  Error code, if any.
		 */
		virtual int PushFloat(float number) = 0;
	
		/**
		 * @brief Pushes a float onto the current call by reference.
		 * NOTE: On Execute, the pointer passed will be modified if copyback is enabled.
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *	 This means you cannot push a pointer, change it, and push it again and expect
		 *	   two different values to come out.
		 *
		 * @param number  Parameter value to push.
		 * @param flags	Copy-back flags.
		 * @return	  Error code, if any.
		 */
		virtual int PushFloatByRef(float* number, int flags = SM_PARAM_COPYBACK) = 0;
	
		/**
		 * @brief Pushes a string or string buffer.
		 *
		 * NOTE: On Execute, the pointer passed will be modified if copy-back is enabled.
		 *
		 * @param buffer  Pointer to string buffer.
		 * @param length  Length of buffer.
		 * @param sz_flags  String flags.  In copy mode, the string will be copied
		 *		  according to the handling (ascii, utf-8, binary, etc).
		 * @param cp_flags  Copy-back flags.
		 * @return	  Error code, if any.
		 */
		virtual int PushStringEx(char* buffer, size_t length, int sz_flags, int cp_flags) = 0;
	
		/**
		 * @brief Cancels a function call that is being pushed but not yet executed.
		 * This can be used be reset for CallFunction() use.
		 */
		virtual void Cancel() = 0;
	
		/**
		 * @brief Pushes an int64 value onto the current call.
		 *
		 * @param number	Parameter to push.
		 * @return		  Error code, if any.
		 */
		virtual int PushInt64(int64_t value) = 0;

		/**
		 * @brief Alternate version of Execute() that uses CallArgs rather than
		 * individual Push methods.
		 *
		 * Each Push() method on CallArgs supports the same behavior as the
		 * Push methods on IForward.
		 *
		 * Unlike IPluginFunction::Invoke, this does not automatically propagate
		 * an exception.
		 */
		virtual int Execute(const sp::CallArgs& args, cell_t *result = nullptr, IForwardFilter *filter = nullptr) = 0;
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

#endif //_INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
