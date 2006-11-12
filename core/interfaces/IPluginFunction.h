#ifndef _INCLUDE_SOURCEMOD_PLUGINFUNCTION_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINFUNCTION_INTERFACE_H_

#include <IPluginSys.h>

namespace SourceMod
{
	#define SM_FUNCFLAG_COPYBACK		(1<<0)		/* Copy an array/reference back after call */

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
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *		 This means you cannot push a pointer, change it, and push it again and expect
		 *       two different values to come out.
		 *
		 * @param float		Parameter value to push.
		 & @param flags		Copy-back flags.
		 * @return			Error code, if any.
		 */
		virtual int PushFloatByRef(float *number, int flags) =0;

		/**
		 * @brief Pushes an array of cells onto the current call.  
		 * NOTE: On Execute, the pointer passed will be modified if non-NULL and copy-back
		 * is enabled.
		 * NOTE: By reference parameters are cached and thus are not read until execution.
		 *		 This means you cannot push a pointer, change it, and push it again and expect
		 *       two different values to come out.
		 *
		 * @param array		Array to copy, NULL if no initial array should be copied.
		 * @param cells		Number of cells to allocate and optionally read from the input array.
		 * @param phys_addr	Optional return address for physical array, if one was made.
		 * @param flags		Whether or not changes should be copied back to the input array.
		 * @return			Error code, if any.
		 */
		virtual int PushArray(cell_t *inarray, 
								unsigned int cells, 
								cell_t **phys_addr, 
								int flags=0) =0;

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
		virtual int PushStringEx(char *string, int flags) =0;

		/**
		 * @brief Cancels a function call that is being pushed but not yet executed.
		 * This can be used be reset for CallFunction() use.
		 */
		virtual void Cancel() =0;
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
		virtual int Execute(cell_t *result) =0;

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
