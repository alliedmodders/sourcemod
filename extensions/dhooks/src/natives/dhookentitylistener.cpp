#include "../globals.hpp"
#include "../sp_inc.hpp"

#include <unordered_map>
#include <unordered_set>

namespace dhooks::natives::dhookentitylistener {

namespace locals {

std::unordered_map<SourcePawn::IPluginContext*, std::unordered_set<SourcePawn::IPluginFunction*>> sm_plugin_callbacks_created;
std::unordered_map<SourcePawn::IPluginContext*, std::unordered_set<SourcePawn::IPluginFunction*>> sm_plugin_callbacks_deleted;

class LocalListener : public SourceMod::ISMEntityListener {
	virtual void OnEntityCreated(CBaseEntity* entity, const char* classname) override {
		if (!entity) {
			return;
		}

		auto ref = globals::gamehelpers->EntityToBCompatRef(entity);
		auto length = strlen(classname) + 1;
		cell_t ignore;

		for (const auto& plugin_callbacks : sm_plugin_callbacks_created) {
			for (const auto& callback : plugin_callbacks.second) {
				callback->PushCell(ref);
				callback->PushStringEx((char*)classname, length, SM_PARAM_STRING_COPY, 0);
				callback->Execute(&ignore);
			}
		}
	}

	virtual void OnEntityDestroyed(CBaseEntity* entity) override {
		if (!entity) {
			return;
		}

		auto ref = globals::gamehelpers->EntityToBCompatRef(entity);
		cell_t ignore;

		for (const auto& plugin_callbacks : sm_plugin_callbacks_deleted) {
			for (const auto& callback : plugin_callbacks.second) {
				callback->PushCell(ref);
				callback->Execute(&ignore);
			}
		}
	}
} sm_listener;
}

void add_listener(SourceMod::ISDKHooks* sdkhooks) {
	sdkhooks->AddEntityListener(&locals::sm_listener);
}

void remove_listener(SourceMod::ISDKHooks* sdkhooks) {
	sdkhooks->RemoveEntityListener(&locals::sm_listener);
}

void remove_plugin(SourcePawn::IPluginContext* context) {
	locals::sm_plugin_callbacks_created.erase(context);
	locals::sm_plugin_callbacks_deleted.erase(context);
}

cell_t DHookAddEntityListener(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto type = static_cast<sp::ListenType>(params[1]);
	auto callback = context->GetFunctionById(params[2]);
	if (callback == nullptr) {
		return context->ThrowNativeError("Invalid listen callback %d !", params[2]);
	}

	if (type == sp::ListenType_Created || type == sp::ListenType_Deleted) {
		auto& callbacks = (type == sp::ListenType_Created) ? locals::sm_plugin_callbacks_created : locals::sm_plugin_callbacks_deleted;

		auto plugin_it = callbacks.find(context);
		if (plugin_it == callbacks.end()) {
			auto plugin_insert = callbacks.try_emplace(context);
			if (!plugin_insert.second) {
				return context->ThrowNativeError("Failed to insert entity listener, contact a Sourcemod developer!", params[2]);
			}
			plugin_it = plugin_insert.first;
		}

		plugin_it->second.insert(callback);
	} else {
		return context->ThrowNativeError("Unknown listen type %d !", type);
	}
	return 0;
}

cell_t DHookRemoveEntityListener(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto type = static_cast<sp::ListenType>(params[1]);
	auto callback = context->GetFunctionById(params[2]);
	if (callback == nullptr) {
		return context->ThrowNativeError("Invalid listen callback %d !", params[2]);
	}

	if (type == sp::ListenType_Created || type == sp::ListenType_Deleted) {
		auto& callbacks = (type == sp::ListenType_Created) ? locals::sm_plugin_callbacks_created : locals::sm_plugin_callbacks_deleted;

		auto plugin_it = callbacks.find(context);
		if (plugin_it == callbacks.end()) {
			return 0;
		}

		// Returns 0 if nothing deleted, anything else otherwise
		return plugin_it->second.erase(callback);
	} else {
		return context->ThrowNativeError("Unknown listen type %d !", type);
	}
}

void init(std::vector<sp_nativeinfo_t>& natives) {
	sp_nativeinfo_t list[] = {
		{"DHookAddEntityListener",           DHookAddEntityListener},
		{"DHookRemoveEntityListener",        DHookRemoveEntityListener}
	};
	natives.insert(natives.end(), std::begin(list), std::end(list));
}

}