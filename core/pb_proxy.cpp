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
#include "pb_proxy.h"

#include <stdlib.h>
#include <sm_platform.h>

#if SOURCE_ENGINE == SE_CSGO
# include <cstrike15_usermessage_helpers.h>
#else
# error "No source engine compatibility"
#endif

class ProtobufProxy : public IProtobufProxy
{
public:
	bool Serialize(const google::protobuf::Message* message, void** out, size_t* len);
	void FreeBuffer(void* data);
	bool Deserialize(const void* buffer, size_t len, google::protobuf::Message* message);
	google::protobuf::Message* NewPrototype(int msg_type);
	void FreeMessage(google::protobuf::Message* message);
};

static ProtobufProxy sProtobufProxy;

PLATFORM_EXTERN_C IProtobufProxy*
GetProtobufProxy()
{
	return &sProtobufProxy;
}

bool
ProtobufProxy::Serialize(const google::protobuf::Message* message, void** out, size_t* len)
{
	*len = message->ByteSize();
	*out = malloc(*len);
	if (!*out)
		return false;
	if (!message->SerializePartialToArray(*out, *len)) {
		free(*out);
		return false;
	}
	return true;
}

void
ProtobufProxy::FreeBuffer(void* data)
{
	free(data);
}

bool
ProtobufProxy::Deserialize(const void* buffer, size_t len, google::protobuf::Message* message)
{
	return message->ParsePartialFromArray(buffer, len);
}

google::protobuf::Message*
ProtobufProxy::NewPrototype(int msg_type)
{
#if SOURCE_ENGINE == SE_CSGO
	return g_Cstrike15UsermessageHelpers.GetPrototype(msg_type)->New();
#else
# error "No source engine compatibility."
#endif
}

void
ProtobufProxy::FreeMessage(google::protobuf::Message* message)
{
	delete message;
}
