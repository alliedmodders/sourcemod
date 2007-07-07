#include "extension.h"
#include "CellRecipientFilter.h"

static cell_t PrefetchSound(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	engsound->PrefetchSound(name);

	return 1;
}

static cell_t GetSoundDuration(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	return sp_ftoc(engsound->GetSoundDuration(name));
}

static cell_t EmitAmbientSound(IPluginContext *pContext, const cell_t *params)
{
	cell_t entity;
	Vector pos;
	char *name;
	float vol, delay;
	int pitch, flags, level;

	entity = params[3];

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	pos.x = sp_ctof(addr[0]);
	pos.y = sp_ctof(addr[1]);
	pos.z = sp_ctof(addr[2]);

	pContext->LocalToString(params[1], &name);

	vol = sp_ctof(params[6]);
	level = params[4];
	flags = params[5];
	pitch = params[7];
	delay = sp_ctof(params[8]);

	engine->EmitAmbientSound(entity, pos, name, vol, (soundlevel_t)level, flags, pitch, delay);

	return 1;
}

static cell_t FadeClientVolume(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	if (client < 1 || client > playerhelpers->GetMaxClients())
	{
		return pContext->ThrowNativeError("Client index %d is not valid", client);
	}

	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client index %d is not in game", client);
	}

	engine->FadeClientVolume(player->GetEdict(),
		sp_ctof(params[2]),
		sp_ctof(params[3]),
		sp_ctof(params[4]),
		sp_ctof(params[5]));

	return 1;
}

static cell_t StopSound(IPluginContext *pContext, const cell_t *params)
{
	int entity = params[1];
	int channel = params[2];

	char *name;
	pContext->LocalToString(params[3], &name);

	engsound->StopSound(entity, channel, name);

	return 1;
}

static cell_t EmitSound(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr, *pl_addr;

	CellRecipientFilter crf;
	pContext->LocalToPhysAddr(params[1], &pl_addr);
	crf.Initialize(pl_addr, params[2]);

	char *sample;
	pContext->LocalToString(params[3], &sample);
	
	int entity = params[4];
	int channel = params[5];
	int level = params[6];
	int flags = params[7];
	float vol = sp_ctof(params[8]);
	int pitch = params[9];
	int speakerentity = params[10];

	Vector *pOrigin = NULL, origin;
	Vector *pDir = NULL, dir;

	pContext->LocalToPhysAddr(params[11], &addr);
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		pOrigin = &origin;
		origin.x = sp_ctof(addr[0]);
		origin.y = sp_ctof(addr[1]);
		origin.z = sp_ctof(addr[2]);
	}

	pContext->LocalToPhysAddr(params[12], &addr);
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		pDir = &dir;
		dir.x = sp_ctof(addr[0]);
		dir.y = sp_ctof(addr[1]);
		dir.z = sp_ctof(addr[2]);
	}

	bool updatePos = params[13] ? true : false;
	float soundtime = sp_ctof(params[14]);

	CUtlVector<Vector> *pOrigVec = NULL;
	CUtlVector<Vector> origvec;
	if (params[0] > 14)
	{
		pOrigVec = &origvec;
		for (cell_t i = 15; i <= params[0]; i++)
		{
			Vector vec;
			pContext->LocalToPhysAddr(params[i], &addr);
			vec.x = sp_ctof(addr[0]);
			vec.y = sp_ctof(addr[1]);
			vec.z = sp_ctof(addr[2]);
			origvec.AddToTail(vec);
		}
	}

	/* If we're going to a "local player" and this is a dedicated server,
	 * intelligently redirect each sound.
	 */

	if (entity == -2 && engine->IsDedicatedServer())
	{
		for (cell_t i=0; i<params[2]; i++)
		{
			cell_t player[1];
			player[0] = pl_addr[i];
			crf.Reset();
			crf.Initialize(player, 1);
			engsound->EmitSound(crf, 
				player[0], 
				channel, 
				sample, 
				vol, 
				(soundlevel_t)level, 
				flags, 
				pitch, 
				pOrigin,
				pDir,
				pOrigVec,
				updatePos,
				soundtime,
				speakerentity);
		}
	} else {
		engsound->EmitSound(crf, 
			entity, 
			channel, 
			sample, 
			vol, 
			(soundlevel_t)level, 
			flags, 
			pitch, 
			pOrigin,
			pDir,
			pOrigVec,
			updatePos,
			soundtime,
			speakerentity);
	}

	return 1;
}

static cell_t EmitSentence(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;

	CellRecipientFilter crf;
	pContext->LocalToPhysAddr(params[1], &addr);
	crf.Initialize(addr, params[2]);

	int sentence = params[3];
	int entity = params[4];
	int channel = params[5];
	int level = params[6];
	int flags = params[7];
	float vol = sp_ctof(params[8]);
	int pitch = params[9];
	int speakerentity = params[10];

	Vector *pOrigin = NULL, origin;
	Vector *pDir = NULL, dir;

	pContext->LocalToPhysAddr(params[11], &addr);
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		pOrigin = &origin;
		origin.x = sp_ctof(addr[0]);
		origin.y = sp_ctof(addr[1]);
		origin.z = sp_ctof(addr[2]);
	}

	pContext->LocalToPhysAddr(params[12], &addr);
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		pDir = &dir;
		dir.x = sp_ctof(addr[0]);
		dir.y = sp_ctof(addr[1]);
		dir.z = sp_ctof(addr[2]);
	}

	bool updatePos = params[13] ? true : false;
	float soundtime = sp_ctof(params[14]);

	CUtlVector<Vector> *pOrigVec = NULL;
	CUtlVector<Vector> origvec;
	if (params[0] > 14)
	{
		pOrigVec = &origvec;
		for (cell_t i = 15; i <= params[0]; i++)
		{
			Vector vec;
			pContext->LocalToPhysAddr(params[i], &addr);
			vec.x = sp_ctof(addr[0]);
			vec.y = sp_ctof(addr[1]);
			vec.z = sp_ctof(addr[2]);
			origvec.AddToTail(vec);
		}
	}

	engsound->EmitSentenceByIndex(crf, 
		entity, 
		channel, 
		sentence, 
		vol, 
		(soundlevel_t)level, 
		flags, 
		pitch, 
		pOrigin,
		pDir,
		pOrigVec,
		updatePos,
		soundtime,
		speakerentity);

	return 1;
}

sp_nativeinfo_t g_SoundNatives[] = 
{
	{"EmitAmbientSound",	EmitAmbientSound},
	{"EmitSentence",		EmitSentence},
	{"EmitSound",			EmitSound},
	{"FadeClientVolume",	FadeClientVolume},
	{"GetSoundDuration",	GetSoundDuration},
	{"PrefetchSound",		PrefetchSound},
	{"StopSound",			StopSound},
	{NULL,					NULL},
};
