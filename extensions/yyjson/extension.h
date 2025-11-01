#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include <yyjson.h>
#include <random>
#include "IYYJSONManager.h"

class JsonExtension : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
};

class JSONHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object);
};

extern JsonExtension g_JsonExtension;
extern HandleType_t g_htJSON;
extern JSONHandler g_JSONHandler;
extern const sp_nativeinfo_t json_natives[];
extern IYYJSONManager* g_pYYJSONManager;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_