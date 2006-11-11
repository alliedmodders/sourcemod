#ifndef _INCLUDE_SOURCEMOD_PLUGINFUNCTION_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINFUNCTION_INTERFACE_H_

#include <IPluginSys.h>

namespace SourceMod
{
	#define SMFUNC_COPYBACK_NONE	(0)			/* Never copy an array back */
	#define SMFUNC_COPYBACK_ONCE	(1<<0)		/* Copy an array back after call */
	#define SMFUNC_COPYBACK_ALWAYS	(1<<1)		/* Copy an array back after subsequent calls (forwards) */

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
		 *
		 * @param cell		Address containing parameter value to push.
		 * @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushCellByRef(cell_t *cell, int flags) =0;

		/**
		 * @brief Pushes a float onto the current call.
		 *
		 * @param float		Parameter value to push.
		 * @return			Error code, if any.
		 */
		virtual int PushFloat(float number) =0;

		/**
		 * @brief Pushes a float onto the current call by reference.
		 * NOTE: On Execute, the pointer passed will be modified if copyback is enabled.
		 *
		 * @param float		Parameter value to push.
		 & @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushFloatByRef(float *number, int flags) =0;

		/**
		 * @brief Pushes an array of cells onto the current call, each cell individually.
		 * NOTE: This is essentially a crippled version of PushArray().
		 *
		 * @param array		Array of cells.
		 * @param numcells	Number of cells in array.
		 * @param each		Whether or not to push as an array or individual cells.
		 * @return			Error code, if any.
		 */
		virtual int PushCells(cell_t array[], unsigned int numcells, bool each) =0;

		/**
		 * @brief Pushes an array of cells onto the current call.  
		 * NOTE: On Execute, the pointer passed will be modified if non-NULL and copy-back
		 * is enabled.
		 *
		 * @param array		Array to copy, NULL if no initial array should be copied.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param phys_addr	Optional return address for physical array (if array was non-NULL, will be array).
		 * @param flags		Whether or not changes should be copied back to the input array.
		 * @return			Error code, if any.
		 */
		virtual int PushArray(cell_t *inarray, 
								unsigned int cells, 
								cell_t **phys_addr, 
								int flags=SMFUNC_COPYBACK_NONE) =0;

		/**
		 * @brief Pushes a string onto the current call.
		 *
		 * @param string	String to push.
		 * @return			Error code, if any.
		 */
		virtual int PushString(const char *string) =0;

		/**
		 * @brief Pushes a string onto the current call.
		 * NOTE: On Execute, the pointer passed will be modified if copy-back is enabled.
		 *
		 * @param string	String to push.
		 * @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushStringByRef(char *string, int flags) =0;

		/**
		 * @brief Cancels a function call that is being pushed but not yet executed.
		 * This can be used be reset for CallFunction() use.
		 */
		virtual void Cancel() =0;
	};


	/**
	 * @brief Used for copy-back notification
	 */
	class IFunctionCopybackReader
	{
	public:
		/**
		 * @brief Called before an array is copied back.
		 * 
		 * @param param			Parameter index.
		 * @param cells			Number of cells in the array.
		 * @param source_addr	Source address in the plugin that will be copied.
		 * @param orig_addr		Destination addres in plugin that will be overwritten.
		 * @param flags			Copy flags.
		 * @return				True to copy back, false otherwise.
		 */
		virtual bool OnCopybackArray(unsigned int param, 
								unsigned int cells,
								cell_t *source_addr,
								cell_t *orig_addr,
								int flags) =0;
	};

	/**
	 * @brief Encapsulates a function call in a plugin.
	 * NOTE: Function calls must be atomic to one execution context.
	 * NOTE: This object should not be deleted.  It lives for the lifetime of the plugin.
	 */
	class IPluginFunction : public ICallable
	{
	public:
		/**
		 * @brief Executes the forward, resets the pushed parameter list, and performs any copybacks.
		 *
		 * @param result		Pointer to store return value in.
		 * @param reader		Copy-back listener.  NULL to specify 
		 * @return				Error code, if any.
		 */
		virtual int Execute(cell_t *result, IFunctionCopybackReader *reader) =0;

		/**
		 * @brief Executes the function with the given parameter array.
		 * Parameters are read in forward order (i.e. index 0 is parameter #1)
		 * NOTE: You will get an error if you attempt to use CallFunction() with
		 * previously pushed parameters.
		 *
		 * @param param			Array of cell parameters.
		 * @param num_params	Number of parameters to push.
		 * @param result		Pointer to store result of function on return.
		 * @return				SourcePawn error code (if any).
		 */
		virtual int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result) =0;

		/**
		 * @brief Returns which plugin this function belongs to.
		 *
		 * @return				IPlugin pointer to parent plugin.
		 */
		virtual IPlugin *GetParentPlugin() =0;

		/**
		 * @brief Returns the physical address of a by-reference parameter.
		 *
		 * @param				Parameter index to read (beginning at 0).
		 * @return				Address, or NULL if invalid parameter specified.
		 */
		virtual cell_t *GetAddressOfPushedParam(unsigned int param) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_PLUGINFUNCTION_INTERFACE_H_
