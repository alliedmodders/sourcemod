#ifndef _INCLUDE_SOURCEMOD_VALVE_CALLER_H_
#define _INCLUDE_SOURCEMOD_VALVE_CALLER_H_

#include <sh_stack.h>
#include <extensions/IBinTools.h>
#include "vdecoder.h"

using namespace SourceMod;
using namespace SourceHook;

/**
 * @brief Valve pre-defined calling types
 */
enum ValveCallType
{
	ValveCall_Static,	/**< Static call */
	ValveCall_Entity,	/**< Thiscall (CBaseEntity implicit first parameter) */
	ValveCall_Player,	/**< Thiscall (CBasePlayer implicit first parameter) */
};

/**
 * @brief Valve parameter info
 */
struct ValvePassInfo
{
	ValveType vtype;		/**< IN: Valve type */
	unsigned int decflags;	/**< IN: VDECODE_FLAG_* */
	unsigned int encflags;	/**< IN: VENCODE_FLAG_* */
	PassType type;			/**< IN: Pass information */
	unsigned int flags;		/**< IN: Pass flags */
	size_t offset;			/**< OUT: stack offset */
};

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
	unsigned char *retbuf;		/**< Return buffer */
	CStack<unsigned char *> stk; /**< Parameter stack */

	unsigned char *stk_get();
	void stk_put(unsigned char *ptr);
	~ValveCall();
};

ValveCall *CreateValveVCall(unsigned int vtableIdx,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams);

#endif //_INCLUDE_SOURCEMOD_VALVE_CALLER_H_
