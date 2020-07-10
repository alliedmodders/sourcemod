/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2019 AlliedModders LLC.  All rights reserved.
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

#include <cstdlib>

#include "sm_platform.h"
#ifndef PLATFORM_WINDOWS
	#error InvalidParameterHandler included in non-Windows build
#endif

/**
 * Allows easy CRT invalid parameter redirection, previous handler is reset upon object destruction.
 */
class InvalidParameterHandler {
public:
	InvalidParameterHandler() {
		this->old = _set_invalid_parameter_handler([](const wchar_t * expression, const wchar_t * function,  const wchar_t * file,  unsigned int line, uintptr_t pReserved) {
			return;
		});
	}
	InvalidParameterHandler(_invalid_parameter_handler newhandler) {
		this->old = _set_invalid_parameter_handler(newhandler);
	}
	~InvalidParameterHandler() {
		_set_invalid_parameter_handler(this->old);
	}

	// explicitly disable copy cstr and assignment
	InvalidParameterHandler(const InvalidParameterHandler &) = delete;
	InvalidParameterHandler& operator=(const InvalidParameterHandler &) = delete;

private:
	_invalid_parameter_handler old;
};
