#include <extension.h>

#define SPEAK_NORMAL		0
#define SPEAK_MUTED			1
#define SPEAK_ALL			2
#define SPEAK_LISTENALL		4

size_t g_VoiceFlags[65];
size_t g_VoiceFlagsCount = 0;

SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);

bool SDKTools::OnSetClientListening(int iReceiver, int iSender, bool bListen)
{
	if (g_VoiceFlags[iSender] & SPEAK_MUTED) 
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}

	if (g_VoiceFlags[iSender] & SPEAK_ALL)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if (g_VoiceFlags[iReceiver] & SPEAK_LISTENALL)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	RETURN_META_VALUE(MRES_IGNORED, bListen);
}

void SDKTools::OnClientDisconnecting(int client)
{
	if (g_VoiceFlags[client])
	{	
		g_VoiceFlags[client] = 0;
		if (!--g_VoiceFlagsCount)
		{
			SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_SdkTools, &SDKTools::OnSetClientListening, false);
		}
	}
}

static cell_t SetClientListeningFlags(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	} else if (!player->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	if (!params[2] && g_VoiceFlags[params[1]])
	{
		if (!--g_VoiceFlagsCount)
		{
			SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_SdkTools, &SDKTools::OnSetClientListening, false);
		}
	} else if (!g_VoiceFlags[params[1]] && params[2]) {

		if (!g_VoiceFlagsCount++)
		{
			SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_SdkTools, &SDKTools::OnSetClientListening, false);
		}
	}

	g_VoiceFlags[params[1]] = params[2];

	return 1;
}

static cell_t GetClientListeningFlags(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	} else if (!player->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	return g_VoiceFlags[params[1]];
}

static cell_t SetClientListening(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	} else if (!player->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	bool bListen = !params[3] ? false : true;

	return voiceserver->SetClientListening(params[1], params[2], bListen) ? 1 : 0;
}

static cell_t GetClientListening(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	} else if (!player->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	return voiceserver->GetClientListening(params[1], params[2]) ? 1 : 0;
}

sp_nativeinfo_t g_VoiceNatives[] = 
{
	{"SetClientListeningFlags",		SetClientListeningFlags},
	{"GetClientListeningFlags",		GetClientListeningFlags},
	{"SetClientListening",			SetClientListening},
	{"GetClientListening",			GetClientListening},
	{NULL,							NULL},
};
