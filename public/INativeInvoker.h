/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_INATIVEINVOKER_H_
#define _INCLUDE_SOURCEMOD_INATIVEINVOKER_H_

/**
 * @file INativeInvoker.h
 * @brief Interface for invoking natives.
 */

#include <IShareSys.h>
#include <sp_vm_api.h>

#define SMINTERFACE_NINVOKE_NAME	"INativeInterface"
#define SMINTERFACE_NINVOKE_VERSION	1

#define NINVOKE_DEFAULT_MEMORY		16384

namespace SourceMod
{
	class INativeInvoker : public SourcePawn::ICallable
	{
	public:
		/**
		 * @brief Virtual destructor - use delete to free this.
		 */
		virtual ~INativeInvoker()
		{
		}
	public:
		/**
		 * @brief Begins a native call.
		 *
		 * During a call's preparation, no new calls may be started.
		 *
		 * @param pContext	Context to invoke native under.
		 * @param name		Name of native.
		 * @return			True if native was found, false otherwise.
		 */
		virtual bool Start(SourcePawn::IPluginContext *pContext, const char *name) = 0;

		/**
		 * @brief Invokes the native.  The preparation state is cleared immediately, meaning that 
		 * this object can be re-used after or even from inside the native being called.
		 *
		 * @param result	Optional pointer to retrieve a result.
		 * @return			SP_ERROR return code.
		 */
		virtual int Invoke(cell_t *result) = 0;
	};

	/**
	 * @brief Factory for dealing with native invocations.
	 */
	class INativeInterface : public SMInterface
	{
	public:
		/**
		 * @brief Creates a virtual plugin.  This can be used as an environment to invoke natives.
		 *
		 * IPluginRuntime objects must be freed with the delete operator.
		 *
		 * @param name		Name, or NULL for anonymous.
		 * @param bytes		Number of bytes for memory (NINVOKE_DEFAULT_MEMORY recommended).
		 * @return			New runtime, or NULL on failure.
		 */
		virtual SourcePawn::IPluginRuntime *CreateRuntime(const char *name, size_t bytes) = 0;

		/**
		 * @brief Creates an object that can be used to invoke a single native code.
		 *
		 * @return			New native invoker (free with delete).
		 */
		virtual INativeInvoker *CreateInvoker() = 0;
	};
}

#endif /* _INCLUDE_SOURCEMOD_INATIVEINVOKER_H_ */
