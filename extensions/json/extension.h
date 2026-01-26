#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include "IJsonManager.h"

class JsonExtension : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
};

class JsonHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object);
};

class ArrIterHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object);
};

class ObjIterHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object);
};

extern JsonExtension g_JsonExt;
extern HandleType_t g_JsonType;
extern HandleType_t g_ArrIterType;
extern HandleType_t g_ObjIterType;
extern JsonHandler g_JsonHandler;
extern ArrIterHandler g_ArrIterHandler;
extern ObjIterHandler g_ObjIterHandler;
extern const sp_nativeinfo_t g_JsonNatives[];
extern IJsonManager* g_pJsonManager;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_