#ifndef _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
#define _INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_

#include <IForwardSys.h>
#include <IPluginSys.h>

#define SMINTERFACE_FORWARDMANAGER_NAME		"IForwardManager"
#define SMINTERFACE_FORWARDMANAGER_VERSION	1

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
		ET_Single = 1,		/* Only return the first exec, ignore all others */
		ET_Event = 2,		/* Acts as an event with the ResultTypes above, no mid-Stops allowed, returns highest */
		ET_Hook = 3,		/* Acts as a hook with the ResultTypes above, mid-Stops allowed, returns highest */
	};

	/**
	 * @brief Abstracts multiple function calling.
	 *
	 * NOTE: Parameters should be pushed in forward order, unlike
	 * the virtual machine/IPluginContext order.
	 */
	class IForward : public IPluginFunction
	{
	public:
		/**
		 * @brief Returns the name of the forward.
		 *
		 * @return			Forward name.
		 */
		virtual const char *GetForwardName() =0;

		/**
		 * @brief Pushes a cell onto the current call.
		 *
		 * @param cell		Parameter value to push.
		 * @return			True if successful, false if type or count mismatch.
		 */
		virtual bool PushCell(cell_t cell) =0;

		/**
		 * @brief Pushes a float onto the current call.
		 *
		 * @param float		Parameter value to push.
		 * @return			True if successful, false if type or count mismatch.
		 */
		virtual bool PushFloat(float number) =0;

		/**
		 * @brief Pushes an array of cells onto the current call, each cell individually.
		 * NOTE: This is essentially a crippled version of PushArray().
		 *
		 * @param array		Array of cells.
		 * @param numcells	Number of cells in array.
		 * @param each		Whether or not to push as an array or individual cells.
		 * @return			True if successful, false if too many cells were pushed.
		 */
		virtual bool PushCells(cell_t array[], unsigned int numcells, bool each) =0;

		/**
		 * @brief Pushes an array of cells onto the current call.  
		 *
		 * @param array		Array to copy, NULL if no initial array should be copied.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param phys_addr	Optional return address for physical array.
		 * @param copyback	Whether or not changes should be copied back to the input array.
		 * @return			True if successful, false otherwise.
		 */
		virtual bool PushArray(cell_t *inarray, size_t cells, cell_t **phys_addr, bool copyback) =0;

		/**
		 * @brief Pushes a string onto the current call.
		 *
		 * @param string	String to push.
		 * @return			True if successful, false if type or count mismatch.
		 */
		virtual bool PushString(const char *string) =0;

		/**
		 * @brief Executes the forward, resets the pushed parameter list, and performs any copybacks.
		 *
		 * @param result		Pointer to store return value in (dependent on forward type).
		 * @param last_err		Pointer to store number of successful executions in.
		 * @return				Error code, if any.
		 */
		virtual int Execute(cell_t *result, unsigned int *num_functions) =0;

		/**
		 * @brief Returns the number of functions in this forward.
		 *
		 * @return			Number of functions in forward.
		 */
		virtual unsigned int GetFunctionCount() =0;

		/**
		 * @brief Returns the method of multi-calling this forward has.
		 *
		 * @return			ResultType of the forward.
		 */
		virtual ResultType GetResultType() =0;
	};


	class IChangeableForward : public IForward
	{
	public:
		/** 
		 * @brief Removes a function from the call list.
		 *
		 * @param func		Function to remove.
		 */
		virtual void RemoveFunction(IPluginFunction *func) =0;

		/** 
		 * @brief Removes all instances of a plugin from the call list.
		 *
		 * @param plugin	Plugin to remove instances of.
		 * @return			Number of functions removed therein.
		 */
		virtual void RemoveFunctionsOfPlugin(IPlugin *plugin) =0;

		/**
		 * @brief Adds a function to the call list.
		 *
		 * @param func		Function to add.
		 */
		virtual void AddFunction(IPluginFunction *func) =0;
	};

	enum ParamType
	{
		Param_Any = 0,
		Param_Cell = 1,
		Param_Float = 2,
		Param_String = 3,
		Param_Array = 4,
		Param_VarArgs = 5,
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
		virtual IForward *CreateForward(const char *name, ExecType et, int num_params, ParamType *types, ...) =0;

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

#endif //_INCLUDE_SOURCEMOD_FORWARDINTERFACE_H_
