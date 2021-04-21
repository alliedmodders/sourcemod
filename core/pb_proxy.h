/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2020 AlliedModders LLC.  All rights reserved.
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
#pragma once

#include <google/protobuf/message.h>

class IProtobufProxy
{
public:
	// Serialize the given message into a buffer. The buffer and its length are placed in |out|.
	// The buffer must be freed with FreeBuffer.
	virtual bool Serialize(const google::protobuf::Message* message, void** out, size_t* len) = 0;
	virtual void FreeBuffer(void* data) = 0;

	// Deserialize the given buffer into a message.
	virtual bool Deserialize(const void* buffer, size_t len, google::protobuf::Message* message) = 0;

	// Allocate/free a prototype.
	virtual google::protobuf::Message* NewPrototype(int msg_type) = 0;
	virtual void FreeMessage(google::protobuf::Message* message) = 0;

	bool Serialize(const google::protobuf::Message& message, void** out, size_t* len) {
		return Serialize(&message, out, len);
	}
	bool Deserialize(const void* buffer, size_t len, google::protobuf::Message& message) {
		return Deserialize(buffer, len, &message);
	}
};

typedef IProtobufProxy*(*GetProtobufProxyFn)();
