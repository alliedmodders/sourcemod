#include "extension.h"
#include "JsonManager.h"

JsonExtension g_JsonExt;
SMEXT_LINK(&g_JsonExt);

HandleType_t g_JsonType;
HandleType_t g_ArrIterType;
HandleType_t g_ObjIterType;
JsonHandler g_JsonHandler;
ArrIterHandler g_ArrIterHandler;
ObjIterHandler g_ObjIterHandler;
IJsonManager* g_pJsonManager;

bool JsonExtension::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	sharesys->AddNatives(myself, g_JsonNatives);
	sharesys->RegisterLibrary(myself, "json");

	HandleAccess haJSON;
	handlesys->InitAccessDefaults(nullptr, &haJSON);
	haJSON.access[HandleAccess_Read] = 0;
	haJSON.access[HandleAccess_Delete] = 0;

	HandleError err;
	g_JsonType = handlesys->CreateType("JSON", &g_JsonHandler, 0, nullptr, &haJSON, myself->GetIdentity(), &err);

	if (!g_JsonType) {
		snprintf(error, maxlen, "Failed to create JSON handle type (err: %d)", err);
		return false;
	}

	g_ArrIterType = handlesys->CreateType("JSONArrIter", &g_ArrIterHandler, 0, nullptr, &haJSON, myself->GetIdentity(), &err);
	if (!g_ArrIterType) {
		snprintf(error, maxlen, "Failed to create JSONArrIter handle type (err: %d)", err);
		return false;
	}

	g_ObjIterType = handlesys->CreateType("JSONObjIter", &g_ObjIterHandler, 0, nullptr, &haJSON, myself->GetIdentity(), &err);
	if (!g_ObjIterType) {
		snprintf(error, maxlen, "Failed to create JSONObjIter handle type (err: %d)", err);
		return false;
	}

	if (g_pJsonManager) {
		delete g_pJsonManager;
		g_pJsonManager = nullptr;
	}

	g_pJsonManager = new JsonManager();
	if (!g_pJsonManager) {
		snprintf(error, maxlen, "Failed to create JSON manager instance");
		return false;
	}

	return sharesys->AddInterface(myself, g_pJsonManager);
}

void JsonExtension::SDK_OnUnload()
{
	handlesys->RemoveType(g_JsonType, myself->GetIdentity());
	handlesys->RemoveType(g_ArrIterType, myself->GetIdentity());
	handlesys->RemoveType(g_ObjIterType, myself->GetIdentity());

	if (g_pJsonManager) {
		delete g_pJsonManager;
		g_pJsonManager = nullptr;
	}
}

void JsonHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	delete (JsonValue*)object;
}

void ArrIterHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	delete (JsonArrIter*)object;
}

void ObjIterHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	delete (JsonObjIter*)object;
}