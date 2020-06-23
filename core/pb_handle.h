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

#if defined(PROTOBUF_ENABLE)
#include <google/protobuf/message.h>

#if defined(PROTOBUF_PROXY_ENABLE)
# include "pb_proxy.h"

extern IProtobufProxy* gProtobufProxy;
#endif

const google::protobuf::Message *GetMessagePrototype(int msg_type);

// On SDKs which use protobufs, the engine has objects compiled against a specific
// version of protobuf. Normally this is fine, we take care on Linux to use the
// same C++ ABI. On macOS however, we use libc++ to enable C++11 functionality,
// whereas the protobuf library has been compiled with libstc++. These ABIs are
// not compatible.
//
// To address the problem, we introduce PbHandle. PbHandle is a wrapper around
// protobuf::Message with two added pieces of state: whether or not the handle
// "owns" the message (and can free it in its destructor), and whether or not
// the handle was created by the engine (private) or created by SourceMod
// (local).
//
// Whenever we transfer a protobuf::Message pointer to SourceMod, we must take
// care to convert it to a Local version first. Whenever we transfer a protobuf
// pointer to the engine, we must convert it to a Private handle.
//
// For platforms with no ABI differences (almost all of them), the handle is a
// no-op. The private and local localities are compatible and no translation
// takes place.
//
// On macOS, CS:GO does require translation. SourceMod loads a tiny shim
// library that contains a copy of the protobuf sources compiled against the
// game's ABI. It then provides serialization and deserialization methods.
// SourceMod must not interact with the game's protobuf objects without first
// going through this proxy library.
//
// Note that PbHandle is not quite like unique_ptr_. It can be converted into a
// PbHandle that does not destroy the underlying object. This is mainly because
// UserMessages.cpp has rather complex state, so it is useful to track locality
// without destroying an object. An unowned PbHandle must not outlive the
// owning PbHandle.
class PbHandle
{
public:
	enum Ownership {
		Owned,
		Unowned,
	};
	enum Locality {
		Local,
		Private,
	};

	PbHandle() : msg_(nullptr) {}
	PbHandle(decltype(nullptr)) : msg_(nullptr) {}
	PbHandle(google::protobuf::Message* msg, Ownership ownership, Locality locality)
		: msg_(msg),
		  ownership_(ownership),
		  locality_(locality)
	{}
	PbHandle(PbHandle&& other)
		: msg_(other.msg_),
		  ownership_(other.ownership_),
		  locality_(other.locality_)
	{
		other.msg_ = nullptr;
	}
	PbHandle(const PbHandle&) = delete;

	~PbHandle() {
		maybe_free();
	}

	PbHandle& operator =(PbHandle&& other) {
		if (other.msg_ != msg_)
			maybe_free();
		msg_ = other.msg_;
		ownership_ = other.ownership_;
		locality_ = other.locality_;
		other.msg_ = nullptr;
		return *this;
	}
	PbHandle& operator =(const PbHandle&) = delete;

	google::protobuf::Message* operator ->() const { return msg_; }

	google::protobuf::Message* GetLocalMessage() {
		assert(locality_ == Local);
		return msg_;
	}
	google::protobuf::Message* GetPrivateMessage() {
		assert(locality_ == Private);
		return msg_;
	}

	PbHandle ToLocal(int msg_type) {
		if (locality_ == Local)
			return AsUnowned();
#if defined(PROTOBUF_PROXY_ENABLE)
		PbHandle local(GetMessagePrototype(msg_type)->New(), Owned, Local);
		local.CopyFromPrivate(msg_);
		return local;
#else
		return PbHandle(msg_, Unowned, Local);
#endif
	}

	PbHandle ToPrivate(int msg_type) {
		if (locality_ == Private)
			return AsUnowned();
#if defined(PROTOBUF_PROXY_ENABLE)
		PbHandle priv(gProtobufProxy->NewPrototype(msg_type), Owned, Private);
		priv.CopyFromLocal(msg_);
		return priv;
#else
		return PbHandle(msg_, Unowned, Private);
#endif
	}

	void CopyFrom(const PbHandle& other) {
		if (other.msg_ == msg_) {
			assert(other.locality_ == locality_);
			return;
		}
#if defined(PROTOBUF_PROXY_ENABLE)
		if (other.locality_ == Local)
			CopyFromLocal(other.msg_);
		else if (other.locality_ == Private)
			CopyFromPrivate(other.msg_);
#else
		msg_->CopyFrom(*other.msg_);
#endif
	}

	PbHandle AsUnowned() {
		return PbHandle(msg_, Unowned, locality_);
	}

	bool is_owned() const { return ownership_ == Owned; }
	bool is_local() const { return locality_ == Local; }

private:
#if defined(PROTOBUF_PROXY_ENABLE)
	void CopyFromPrivate(google::protobuf::Message* message) {
		void* out;
		size_t len;
		if (!gProtobufProxy->Serialize(message, &out, &len))
			return;
		if (locality_ == Local)
			msg_->ParsePartialFromArray(out, len);
		else
			gProtobufProxy->Deserialize(out, len, msg_);
		gProtobufProxy->FreeBuffer(out);
	}

	void CopyFromLocal(google::protobuf::Message* message) {
		if (locality_ == Local) {
			msg_->CopyFrom(*message);
		} else {
			auto data = message->SerializePartialAsString();
			gProtobufProxy->Deserialize(data.data(), data.size(), msg_);
		}
	}
#endif

	void maybe_free() {
		if (ownership_ != Owned)
			return;
#if defined(PROTOBUF_PROXY_ENABLE)
		if (locality_ == Private) {
			gProtobufProxy->FreeMessage(msg_);
			return;
		}
#endif
		delete msg_;
	}

private:
	google::protobuf::Message* msg_ = nullptr;
	Ownership ownership_ = Unowned;
	Locality locality_ = Local;
};

#endif // PROTOBUF_ENABLE
