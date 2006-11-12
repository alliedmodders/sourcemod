#ifndef _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
#define _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_

#include <IForwardSys.h>
#include <IPluginSys.h>
#include <IPluginFunction.h>

#define SMINTERFACE_FORWARDMANAGER_NAME		"IForwardManager"
#define SMINTERFACE_FORWARDMANAGER_VERSION	1

/**
 * There is some very important documentation at the bottom of this file.
 * Readers interested in knowing more about the forward system, scrolling down is a must!
 */

namespace SourceMod
{
	enum ResultType
	{
		Pl_Continue = 0,	/* No result */
		Pl_Handled = 1,		/* Result was handled, stop at the end */
		Pl_Stop = 2,		/* Result was handled, stop now */
	};

	enum ExecType
	{
		ET_Ignore = 0,		/* Ignore all return values, return 0 */
		ET_Single = 1,		/* Only return the last exec, ignore all others */
		ET_Event = 2,		/* Acts as an event with the ResultTypes above, no mid-Stops allowed, returns highest */
		ET_Hook = 3,		/* Acts as a hook with the ResultTypes above, mid-Stops allowed, returns highest */
		ET_Custom = 4,		/* Ignored or handled by an IForwardFilter */
	};

	class IForward;

	class IForwardFilter
	{
	public:
		/**
		 * @brief Called when an error occurs executing a plugin.
		 *
		 * @param fwd		IForward pointer.
		 * @param func		IPluginFunction pointer to the failed function.
		 * @param err		Error code.
		 * @return			True to handle, false to pass to global error reporter.
		 */
		virtual bool OnErrorReport(IForward *fwd,
									IPluginFunction *func,
									int err)
		{
			return false;
		}

		/** 
		 * @brief Called after each function return during execution.
		 * NOTE: Only used for ET_Custom.
		 *
		 * @param fwd		IForward pointer.
		 * @param func		IPluginFunction pointer to the executed function.
		 * @param retval	Pointer to current return value (can be modified).
		 * @return			ResultType denoting the next action to take.
		 */
		virtual ResultType OnFunctionReturn(IForward *fwd,
									  IPluginFunction *func,
									  cell_t *retval)
		{
			return Pl_Continue;
		}

		/** 
		 * @brief Called when execution begins.
		 */
		virtual void OnExecuteBegin()
		{
		};

		/**
		 * @brief Called when execution ends.
		 * 
		 * @param final_ret	Final return value (modifiable).
		 * @param success	Number of successful execs.
		 * @param failed	Number of failed execs.
		 */
		virtual void OnExecuteEnd(cell_t *final_ret, unsigned int success, unsigned int failed)
		{
		}
	};

	/**
	 * @brief Abstracts multiple function calling.
	 * 
	 * NOTE: Parameters should be pushed in forward order, unlike
	 *       the virtual machine/IPluginContext order.
	 * NOTE: Some functions are repeated in here because their 
	 *       documentation differs from their IPluginFunction equivalents.
	 *       Missing are the Push functions, whose only doc change is that 
	 *       they throw SP_ERROR_PARAM on type mismatches.
	 */
	class IForward : public ICallable
	{
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
		 * @param result		Pointer to store result in.
		 * @param filter		Optional pointer to an IForwardFilter.
		 * @return				Error code, if any.
		 */
		virtual int Execute(cell_t *result, IForwardFilter *filter=NULL) =0;

		/**
		 * @brief Pushes an array of cells onto the current call.  Different rules than ICallable.
		 * NOTE: On Execute, the pointer passed will be modified according to the copyback rule.
		 *
		 * @param array		Array to copy.  Cannot be NULL, unlike ICallable's version.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param phys_addr	Unused.  If a value is passed, it will be filled with NULL.
		 * @param flags		Whether or not changes should be copied back to the input array.
		 * @return			Error code, if any.
		 */
		virtual int PushArray(cell_t *inarray, 
								unsigned int cells, 
								cell_t **phys_addr, 
								int flags=SMFUNC_COPYBACK_NONE) =0;
	};


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
		 * NOTE: Cannot be used during an incompleted call.
		 * NOTE: If used during a call, function is temporarily queued until calls are over.
		 * NOTE: Adding mulitple copies of the same function is illegal.
		 *
		 * @param func		Function to add.
		 * @return			True on success, otherwise false.
		 */
		virtual bool AddFunction(IPluginFunction *func) =0;

		/**
		 * @brief Adds a function to the call list.
		 * NOTE: Cannot be used during an incompleted call.
		 * NOTE: If used during a call, function is temporarily queued until calls are over.
		 *
		 * @param ctx		Context to use as a look-up.
		 * @param funcid	Function id to add.
		 * @return			True on success, otherwise false.
		 */
		virtual bool AddFunction(sp_context_t *ctx, funcid_t index) =0;
	};

	#define SP_PARAMTYPE_ANY	0
	#define SP_PARAMFLAG_BYREF	(1<<0)
	#define SP_PARAMTYPE_CELL	(1<<1)
	#define SP_PARAMTYPE_FLOAT	(2<<1)
	#define SP_PARAMTYPE_STRING	(3<<1)|SP_PARAMFLAG_BYREF
	#define SP_PARAMTYPE_ARRAY	(4<<1)|SP_PARAMFLAG_BYREF
	#define SP_PARAMTYPE_VARARG	(5<<1)

	enum ParamType
	{
		Param_Any = SP_PARAMTYPE_ANY,
		Param_Cell = SP_PARAMTYPE_CELL,
		Param_Float = SP_PARAMTYPE_FLOAT,
		Param_String = SP_PARAMTYPE_STRING,
		Param_Array = SP_PARAMTYPE_ARRAY,
		Param_VarArgs = SP_PARAMTYPE_VARARG,
		Param_CellByRef = SP_PARAMTYPE_CELL|SP_PARAMFLAG_BYREF,
		Param_FloatByRef = SP_PARAMTYPE_FLOAT|SP_PARAMFLAG_BYREF,
	};
	
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
										ParamType *types, 
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
													ParamType *types, 
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
	};
};

/**
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
 * issue, rather than a usability one.  The interesting part is when it gets to implementation, which is when the 
 * problems for SourceMod arise.  Specifically, the implementation is easier in GENERAL -- the tough part is the oddball
 * cases.
 *
 * Observe the calling process:
 * - Each parameter is pushed using the ICallable interface.  For each parameter:
 *  - For each function in the collection, the parameter is processed and pushed.
 * - For each function in the collection:
 *  - The call is made.
 *  - Copy backs are performed.
 * - Return
 *
 *  Astute readers will note the problems - the parameters are processed individually for each push,
 * rather than for each call.  This means:
 * 1) More memory is used.  Specifically, rather than N params of memory, you now have N params * M plugins.
 *    This is because, again, parameters are processed per function object, per-push, before the call.
 * 2) There are slightly more calls going around: one extra call for each parameter, since each push is manual.
 * 3) Copybacks won't work as expected.  
 *
 *  Number 3 is hard to see.  For example, say a forward has two functions, and an array is pushed with copyback.
 * The array gets pushed and copied into each plugin.  When the call is made, each plugin now has separate copies of 
 * the array.  When the copyback is performed, it gets mirrored to the originall address, but not to the next plugin!

 *  Implementing this is "difficult."  To be re-entrant, an IPluginFunction can't tell you anything
 * about its copybacks after it has returned, because its internal states were reset long ago.
 * 
 *  In order to implement this feature, IPluginFunction::Execute function now lets you pass a listener in.
 * This listener is notified each time an array parameter is about to be copied back.  Specifically, the forward 
 * uses this to detect when it needs to push a new parameter out to the next plugin.  When the forward gets the 
 * call back, it detects whether there is another plugin in the queue.  If there is, it grabs the address it will
 * be using for the same arrays, and specifies it as the new copy-back point.  If no plugins are left, it allows
 * the copyback to chain up to the original address.
 * 
 *  This wonderful little hack is somewhat reminiscent of how SourceHook parameter rewrite chains work.  It seems 
 * ugly at first, but it is actually the correct design pattern to apply to an otherwise awful problem.
 *
 *  Note that there are other solutions to this that aren't based on a visitor-like pattern.  For example, the Function
 * class could expose a "set new original address" function.  Then after arrays are pushed, the arrays are re-browsed,
 * and function #1 gets function #2's original address, function #2 gets function #3's original address, et cetera.
 * This extra browse step is a bit less efficient though, since the "visitor" method implies only taking action when
 * necessary.  Furthermore, it would require exposing such a "set address" function, which should fire a red flag 
 * that the API is doing something it shouldn't (namely, exposing the direct setting of very internal properties).
 */

#endif //_INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
