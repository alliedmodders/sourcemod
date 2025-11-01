#include "extension.h"
#include "YYJSONManager.h"

JsonExtension g_JsonExtension;
SMEXT_LINK(&g_JsonExtension);

HandleType_t g_htJSON;
JSONHandler g_JSONHandler;
IYYJSONManager* g_pYYJSONManager;

bool JsonExtension::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	sharesys->AddNatives(myself, json_natives);
	sharesys->RegisterLibrary(myself, "yyjson");

	HandleAccess haJSON;
	handlesys->InitAccessDefaults(nullptr, &haJSON);
	haJSON.access[HandleAccess_Read] = 0;
	haJSON.access[HandleAccess_Delete] = 0;

	HandleError err;
	g_htJSON = handlesys->CreateType("YYJSON", &g_JSONHandler, 0, nullptr, &haJSON, myself->GetIdentity(), &err);

	if (!g_htJSON) {
		snprintf(error, maxlen, "Failed to create YYJSON handle type (err: %d)", err);
		return false;
	}

	// Delete the existing instance if it exists
	if (g_pYYJSONManager) {
		delete g_pYYJSONManager;
		g_pYYJSONManager = nullptr;
	}

	g_pYYJSONManager = new YYJSONManager();
	if (!g_pYYJSONManager) {
		snprintf(error, maxlen, "Failed to create YYJSONManager instance");
		return false;
	}

	return sharesys->AddInterface(myself, g_pYYJSONManager);
}

void JsonExtension::SDK_OnUnload()
{
	handlesys->RemoveType(g_htJSON, myself->GetIdentity());

	if (g_pYYJSONManager) {
		delete g_pYYJSONManager;
		g_pYYJSONManager = nullptr;
	}
}

void JSONHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	delete (YYJSONValue*)object;
}