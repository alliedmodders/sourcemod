/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SMEXT_BINTOOLS_H_
#define _INCLUDE_SMEXT_BINTOOLS_H_

#include <IShareSys.h>

#define SMINTERFACE_BINTOOLS_NAME		"IBinTools"
#define SMINTERFACE_BINTOOLS_VERSION	2

/**
 * @brief Function calling encoding utilities
 * @file IBinTools.h
 */

namespace SourceMod
{
	/**
	 * @brief Supported calling conventions
	 */
	enum CallConvention
	{
		CallConv_ThisCall,		/**< This call (object pointer required) */
		CallConv_Cdecl,			/**< Standard C call */
	};

	/**
	 * @brief Describes how a parameter should be passed
	 */
	enum PassType
	{
		PassType_Basic,			/**< Plain old register data (pointers, integers) */
		PassType_Float,			/**< Floating point data */
		PassType_Object,		/**< Object or structure */
	};

	#define PASSFLAG_BYVAL		(1<<0)		/**< Passing by value */
	#define PASSFLAG_BYREF		(1<<1)		/**< Passing by reference */
	#define PASSFLAG_ODTOR		(1<<2)		/**< Object has a destructor */
	#define PASSFLAG_OCTOR		(1<<3)		/**< Object has a constructor */
	#define PASSFLAG_OASSIGNOP	(1<<4)		/**< Object has an assignment operator */

	/**
	 * @brief Parameter passing information
	 */
	struct PassInfo
	{
		PassType type;			/**< PassType value */
		unsigned int flags;		/**< Pass/return flags */
		size_t size;			/**< Size of the data being passed */
	};

	/**
	 * @brief Parameter encoding information
	 */
	struct PassEncode
	{
		PassInfo info;			/**< Parameter information */
		size_t offset;			/**< Offset into the virtual stack */
	};

	/**
	 * @brief Wraps a C/C++ call.
	 */
	class ICallWrapper
	{
	public:
		/**
		 * @brief Returns the calling convention.
		 *
		 * @return				CallConvention value.
		 */
		virtual CallConvention GetCallConvention() =0;

		/**
		 * @brief Returns parameter info.
		 *
		 * @param num			Parameter number to get (starting from 0).
		 * @return				A PassInfo pointer.
		 */
		virtual const PassEncode *GetParamInfo(unsigned int num) =0;

		/**
		 * @brief Returns return type info.
		 *
		 * @return				A PassInfo pointer.
		 */
		virtual const PassInfo *GetReturnInfo() =0;

		/**
		 * @brief Returns the number of parameters.
		 *
		 * @return				Number of parameters.
		 */
		virtual unsigned int GetParamCount() =0;

		/**
		 * @brief Execute the contained function.
		 *
		 * @param vParamStack	A blob of memory containing stack data.
		 * @param retBuffer		Buffer to store return value.
		 */
		virtual void Execute(void *vParamStack, void *retBuffer) =0;

		/**
		 * @brief Destroys all resources used by this object.
		 */
		virtual void Destroy() =0;
	};

	/**
	 * @brief Binary tools interface.
	 */
	class IBinTools : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_BINTOOLS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_BINTOOLS_VERSION;
		}
	public:
		/**
		 * @brief Creates a call decoder.
		 *
		 * Note: CallConv_ThisCall requires an implicit first parameter
		 * of PassType_Basic / PASSFLAG_BYVAL / sizeof(void *).  However,
		 * this should only be given to the Execute() function, and never
		 * listed in the paramInfo array.
		 *
		 * @param address			Address to use as a call.
		 * @param cv				Calling convention.
		 * @param retInfo			Return type information, or NULL for void.
		 * @param paramInfo			Array of parameters.
		 * @param numParams			Number of parameters in the array.
		 * @return					A new ICallWrapper function.
		 */
		virtual ICallWrapper *CreateCall(void *address,
											CallConvention cv,
											const PassInfo *retInfo,
											const PassInfo paramInfo[],
											unsigned int numParams) =0;

		/**
		 * @brief Creates a vtable call decoder.
		 *
		 * Note: CallConv_ThisCall requires an implicit first parameter
		 * of PassType_Basic / PASSFLAG_BYVAL / sizeof(void *).  However,
		 * this should only be given to the Execute() function, and never
		 * listed in the paramInfo array.
		 *
		 * @param vtblIdx			Index into the virtual table.
		 * @param vtblOffs			Offset of the virtual table.
		 * @param thisOffs			Offset of the this pointer of the virtual table.
		 * @param retInfo			Return type information, or NULL for void.
		 * @param paramInfo			Array of parameters.
		 * @param numParams			Number of parameters in the array.
		 * @return					A new ICallWrapper function.
		 */
		virtual ICallWrapper *CreateVCall(unsigned int vtblIdx,
											unsigned int vtblOffs,
											unsigned int thisOffs,
											const PassInfo *retInfo,
											const PassInfo paramInfo[],
											unsigned int numParams) =0;
	};
}

#endif //_INCLUDE_SMEXT_BINTOOLS_H_
