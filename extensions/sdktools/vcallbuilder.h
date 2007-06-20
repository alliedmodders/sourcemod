#ifndef _INCLUDE_SOURCEMOD_VALVE_CALLER_H_
#define _INCLUDE_SOURCEMOD_VALVE_CALLER_H_

#include <sh_stack.h>
#include <extensions/IBinTools.h>
#include "vdecoder.h"

using namespace SourceMod;
using namespace SourceHook;

/**
 * @brief Info necessary to call a Valve function
 */
struct ValveCall
{
	ICallWrapper *call;			/**< From IBinTools */
	ValvePassInfo *vparams;		/**< Valve parameter info */
	ValvePassInfo *retinfo;		/**< Return buffer info */
	ValvePassInfo *thisinfo;	/**< Thiscall info */
	size_t stackSize;			/**< Stack size */
	size_t stackEnd;			/**< End of the bintools stack */
	unsigned char *retbuf;		/**< Return buffer */
	CStack<unsigned char *> stk; /**< Parameter stack */

	unsigned char *stk_get();
	void stk_put(unsigned char *ptr);
	ValveCall();
	~ValveCall();
};

ValveCall *CreateValveVCall(unsigned int vtableIdx,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams);

ValveCall *CreateValveCall(void *addr,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams);

#endif //_INCLUDE_SOURCEMOD_VALVE_CALLER_H_
