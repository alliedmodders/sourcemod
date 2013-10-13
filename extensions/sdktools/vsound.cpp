/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include "vsound.h"
#include <IForwardSys.h>

#if SOURCE_ENGINE == SE_DOTA
SH_DECL_HOOK8_void(IVEngineServer, EmitAmbientSound, SH_NOATTRIB, 0, CEntityIndex, const Vector &, const char *, float, soundlevel_t, int, int, float);
#else
SH_DECL_HOOK8_void(IVEngineServer, EmitAmbientSound, SH_NOATTRIB, 0, int, const Vector &, const char *, float, soundlevel_t, int, int, float);
#endif

#if SOURCE_ENGINE >= SE_PORTAL2
SH_DECL_HOOK17(IEngineSound, EmitSound, SH_NOATTRIB, 0, int, IRecipientFilter &, int, int, const char *, unsigned int, const char *, float, float, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
SH_DECL_HOOK17(IEngineSound, EmitSound, SH_NOATTRIB, 1, int, IRecipientFilter &, int, int, const char *, unsigned int, const char *, float, soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
SH_DECL_HOOK15_void(IEngineSound, EmitSound, SH_NOATTRIB, 0, IRecipientFilter &, int, int, const char *, float, float, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
SH_DECL_HOOK15_void(IEngineSound, EmitSound, SH_NOATTRIB, 1, IRecipientFilter &, int, int, const char *, float, soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
#else
SH_DECL_HOOK14_void(IEngineSound, EmitSound, SH_NOATTRIB, 0, IRecipientFilter &, int, int, const char *, float, float, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
SH_DECL_HOOK14_void(IEngineSound, EmitSound, SH_NOATTRIB, 1, IRecipientFilter &, int, int, const char *, float, soundlevel_t, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int);
#endif

bool g_InSoundHook = false;

/***************************
*                          *
* Sound Related Hook Class *
*                          *
****************************/

cell_t SoundReferenceToIndex(cell_t ref)
{
	if (ref == 0 || ref == -1 || ref == -2)
	{
		return ref;
	}

	return gamehelpers->ReferenceToIndex(ref);
}

size_t SoundHooks::_FillInPlayers(int *pl_array, IRecipientFilter *pFilter)
{
	size_t size = static_cast<size_t>(pFilter->GetRecipientCount());

	for (size_t i=0; i<size; i++)
	{
		int index;
#if SOURCE_ENGINE == SE_DOTA
		index = pFilter->GetRecipientIndex(i).Get();
#else
		index = pFilter->GetRecipientIndex(i);
#endif
		pl_array[i] = index;
	}

	return size;
}

void SoundHooks::_IncRefCounter(int type)
{
	if (type == NORMAL_SOUND_HOOK)
	{
		if (m_NormalCount++ == 0)
		{
			SH_ADD_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound), false);
			SH_ADD_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound2), false);
		}
	}
	else if (type == AMBIENT_SOUND_HOOK)
	{
		if (m_AmbientCount++ == 0)
		{
			SH_ADD_HOOK(IVEngineServer, EmitAmbientSound, engine, SH_MEMBER(this, &SoundHooks::OnEmitAmbientSound), false);
		}
	}
}

void SoundHooks::_DecRefCounter(int type)
{
	if (type == NORMAL_SOUND_HOOK)
	{
		if (--m_NormalCount == 0)
		{
			SH_REMOVE_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound), false);
			SH_REMOVE_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound2), false);
		}
	}
	else if (type == AMBIENT_SOUND_HOOK)
	{
		if (--m_AmbientCount == 0)
		{
			SH_REMOVE_HOOK(IVEngineServer, EmitAmbientSound, engine, SH_MEMBER(this, &SoundHooks::OnEmitAmbientSound), false);
		}
	}
}

void SoundHooks::Initialize()
{
	plsys->AddPluginsListener(this);
}

void SoundHooks::Shutdown()
{
	plsys->RemovePluginsListener(this);
	if (m_NormalCount)
	{
		SH_REMOVE_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound), false);
		SH_REMOVE_HOOK(IEngineSound, EmitSound, engsound, SH_MEMBER(this, &SoundHooks::OnEmitSound2), false);
	}
	if (m_AmbientCount)
	{
		SH_REMOVE_HOOK(IVEngineServer, EmitAmbientSound, engine, SH_MEMBER(this, &SoundHooks::OnEmitAmbientSound), false);
	}
}

void SoundHooks::OnPluginUnloaded(IPlugin *plugin)
{
	SoundHookIter iter;
	IPluginContext *pContext = plugin->GetBaseContext();

	if (m_AmbientCount)
	{
		for (iter=m_AmbientFuncs.begin(); iter!=m_AmbientFuncs.end(); )
		{
			if ((*iter)->GetParentContext() == pContext)
			{
				iter = m_AmbientFuncs.erase(iter);
				_DecRefCounter(AMBIENT_SOUND_HOOK);
			}
			else
			{
				iter++;
			}
		}
	}
	if (m_NormalCount)
	{
		for (iter=m_NormalFuncs.begin(); iter!=m_NormalFuncs.end(); )
		{
			if ((*iter)->GetParentContext() == pContext)
			{
				iter = m_NormalFuncs.erase(iter);
				_DecRefCounter(NORMAL_SOUND_HOOK);
			}
			else
			{
				iter++;
			}
		}
	}
}

void SoundHooks::AddHook(int type, IPluginFunction *pFunc)
{
	if (type == NORMAL_SOUND_HOOK)
	{
		m_NormalFuncs.push_back(pFunc);
		_IncRefCounter(NORMAL_SOUND_HOOK);
	}
	else if (type == AMBIENT_SOUND_HOOK)
	{
		m_AmbientFuncs.push_back(pFunc);
		_IncRefCounter(AMBIENT_SOUND_HOOK);
	}
}

bool SoundHooks::RemoveHook(int type, IPluginFunction *pFunc)
{
	SoundHookIter iter;
	if (type == NORMAL_SOUND_HOOK)
	{
		if ((iter=m_NormalFuncs.find(pFunc)) != m_NormalFuncs.end())
		{
			m_NormalFuncs.erase(iter);
			_DecRefCounter(NORMAL_SOUND_HOOK);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (type == AMBIENT_SOUND_HOOK)
	{
		if ((iter=m_AmbientFuncs.find(pFunc)) != m_AmbientFuncs.end())
		{
			m_AmbientFuncs.erase(iter);
			_DecRefCounter(AMBIENT_SOUND_HOOK);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

#if SOURCE_ENGINE == SE_DOTA
void SoundHooks::OnEmitAmbientSound(CEntityIndex index, const Vector &pos, const char *samp, float vol, 
									soundlevel_t soundlevel, int fFlags, int pitch, float delay)
{
	int entindex = index.Get();
#else
void SoundHooks::OnEmitAmbientSound(int entindex, const Vector &pos, const char *samp, float vol, 
									soundlevel_t soundlevel, int fFlags, int pitch, float delay)
{
#endif
	SoundHookIter iter;
	IPluginFunction *pFunc;
	cell_t vec[3] = {sp_ftoc(pos.x), sp_ftoc(pos.y), sp_ftoc(pos.z)};
	cell_t res = static_cast<ResultType>(Pl_Continue);
	char buffer[PLATFORM_MAX_PATH];
	strcpy(buffer, samp);

	for (iter=m_AmbientFuncs.begin(); iter!=m_AmbientFuncs.end(); iter++)
	{
		pFunc = (*iter);
		pFunc->PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&entindex);
		pFunc->PushFloatByRef(&vol);
		pFunc->PushCellByRef(reinterpret_cast<cell_t *>(&soundlevel));
		pFunc->PushCellByRef(&pitch);
		pFunc->PushArray(vec, 3, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&fFlags);
		pFunc->PushFloatByRef(&delay);
		g_InSoundHook = true;
		pFunc->Execute(&res);
		g_InSoundHook = false;

		switch (res)
		{
		case Pl_Handled:
		case Pl_Stop:
			{
				RETURN_META(MRES_SUPERCEDE);
			}
		case Pl_Changed:
			{
				Vector vec2;
				vec2.x = sp_ctof(vec[0]);
				vec2.y = sp_ctof(vec[1]);
				vec2.z = sp_ctof(vec[2]);
#if SOURCE_ENGINE == SE_DOTA
				RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::EmitAmbientSound, 
										(CEntityIndex(entindex), vec2, buffer, vol, soundlevel, fFlags, pitch, delay));
#else
				RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::EmitAmbientSound,
										(entindex, vec2, buffer, vol, soundlevel, fFlags, pitch, delay));
#endif
			}
		}
	}
}

#if SOURCE_ENGINE >= SE_PORTAL2
int SoundHooks::OnEmitSound(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample, 
							 float flVolume, soundlevel_t iSoundlevel, int nSeed, int iFlags, int iPitch, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
void SoundHooks::OnEmitSound(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, 
							 float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch, int iSpecialDSP, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#else
void SoundHooks::OnEmitSound(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, 
							 float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#endif
{
	SoundHookIter iter;
	IPluginFunction *pFunc;
	cell_t res = static_cast<ResultType>(Pl_Continue);
	char buffer[PLATFORM_MAX_PATH];
	strcpy(buffer, pSample);

	for (iter=m_NormalFuncs.begin(); iter!=m_NormalFuncs.end(); iter++)
	{
		int players[64], size;
		size = _FillInPlayers(players, &filter);
		pFunc = (*iter);

		pFunc->PushArray(players, 64, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&size);
		pFunc->PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&iEntIndex);
		pFunc->PushCellByRef(&iChannel);
		pFunc->PushFloatByRef(&flVolume);
		pFunc->PushCellByRef(reinterpret_cast<cell_t *>(&iSoundlevel));
		pFunc->PushCellByRef(&iPitch);
		pFunc->PushCellByRef(&iFlags);
		g_InSoundHook = true;
		pFunc->Execute(&res);
		g_InSoundHook = false;

		switch (res)
		{
		case Pl_Handled:
		case Pl_Stop:
			{
#if SOURCE_ENGINE >= SE_PORTAL2
				RETURN_META_VALUE(MRES_SUPERCEDE, -1);
#else
				RETURN_META(MRES_SUPERCEDE);
#endif
			}
		case Pl_Changed:
			{
				CellRecipientFilter crf;
				crf.Initialize(players, size);
#if SOURCE_ENGINE >= SE_PORTAL2
				RETURN_META_VALUE_NEWPARAMS(
					MRES_IGNORED,
					-1,
					static_cast<int (IEngineSound::*)(IRecipientFilter &, int, int, const char*, unsigned int, const char*, float, soundlevel_t, 
					int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, -1, buffer, flVolume, iSoundlevel, nSeed, iFlags, iPitch, pOrigin, 
					pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
				RETURN_META_NEWPARAMS(
					MRES_IGNORED,
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, soundlevel_t, 
					int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, flVolume, iSoundlevel, iFlags, iPitch, iSpecialDSP, pOrigin, 
					pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#else
				RETURN_META_NEWPARAMS(
					MRES_IGNORED,
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, soundlevel_t, 
					int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, flVolume, iSoundlevel, iFlags, iPitch, pOrigin, 
					pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#endif
			}
		}
	}

#if SOURCE_ENGINE >= SE_PORTAL2
	RETURN_META_VALUE(MRES_IGNORED, -1 );
#endif
}

#if SOURCE_ENGINE >= SE_PORTAL2
int SoundHooks::OnEmitSound2(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample, 
							 float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
void SoundHooks::OnEmitSound2(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, 
							 float flVolume, float flAttenuation, int iFlags, int iPitch, int iSpecialDSP, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#else
void SoundHooks::OnEmitSound2(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, 
							 float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, 
							 const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, 
							 float soundtime, int speakerentity)
#endif
{
	SoundHookIter iter;
	IPluginFunction *pFunc;
	cell_t res = static_cast<ResultType>(Pl_Continue);
	cell_t sndlevel = static_cast<cell_t>(ATTN_TO_SNDLVL(flAttenuation));
	char buffer[PLATFORM_MAX_PATH];
	strcpy(buffer, pSample);

	for (iter=m_NormalFuncs.begin(); iter!=m_NormalFuncs.end(); iter++)
	{
		int players[64], size;
		size = _FillInPlayers(players, &filter);
		pFunc = (*iter);

		pFunc->PushArray(players, 64, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&size);
		pFunc->PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
		pFunc->PushCellByRef(&iEntIndex);
		pFunc->PushCellByRef(&iChannel);
		pFunc->PushFloatByRef(&flVolume);
		pFunc->PushCellByRef(&sndlevel);
		pFunc->PushCellByRef(&iPitch);
		pFunc->PushCellByRef(&iFlags);
		g_InSoundHook = true;
		pFunc->Execute(&res);
		g_InSoundHook = false;

		switch (res)
		{
		case Pl_Handled:
		case Pl_Stop:
			{
#if SOURCE_ENGINE >= SE_PORTAL2
				RETURN_META_VALUE(MRES_SUPERCEDE, -1);
#else
				RETURN_META(MRES_SUPERCEDE);
#endif
			}
		case Pl_Changed:
			{
				CellRecipientFilter crf;
				crf.Initialize(players, size);
#if SOURCE_ENGINE >= SE_PORTAL2
				RETURN_META_VALUE_NEWPARAMS(
					MRES_IGNORED,
					-1,
					static_cast<int (IEngineSound::*)(IRecipientFilter &, int, int, const char *, unsigned int, const char *, float, float, 
					int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, -1, buffer, flVolume, SNDLVL_TO_ATTN(static_cast<soundlevel_t>(sndlevel)), 
					nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
RETURN_META_NEWPARAMS(
					MRES_IGNORED,
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char *, float, float, 
					int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, flVolume, SNDLVL_TO_ATTN(static_cast<soundlevel_t>(sndlevel)), 
					iFlags, iPitch, iSpecialDSP, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#else
				RETURN_META_NEWPARAMS(
					MRES_IGNORED,
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char *, float, float, 
					int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>(&IEngineSound::EmitSound), 
					(crf, iEntIndex, iChannel, buffer, flVolume, SNDLVL_TO_ATTN(static_cast<soundlevel_t>(sndlevel)), 
					iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity)
					);
#endif
			}
		}
	}

#if SOURCE_ENGINE >= SE_PORTAL2
	RETURN_META_VALUE(MRES_IGNORED, -1 );
#endif
}

/************************
*                       *
* Sound Related Natives *
*                       *
*************************/

SoundHooks s_SoundHooks;

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

	entity = SoundReferenceToIndex(params[3]);

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

	if (g_InSoundHook)
	{
		ENGINE_CALL(EmitAmbientSound)(entity, pos, name, vol, (soundlevel_t)level, flags, pitch, delay);
	}
	else
	{
		engine->EmitAmbientSound(entity, pos, name, vol, (soundlevel_t)level, flags, pitch, delay);
	}

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

	engine->FadeClientVolume(
#if SOURCE_ENGINE == SE_DOTA
		player->GetIndex(),
#else
		player->GetEdict(),
#endif
		sp_ctof(params[2]),
		sp_ctof(params[3]),
		sp_ctof(params[4]),
		sp_ctof(params[5]));

	return 1;
}

static cell_t StopSound(IPluginContext *pContext, const cell_t *params)
{
	int entity = SoundReferenceToIndex(params[1]);
	int channel = params[2];

	char *name;
	pContext->LocalToString(params[3], &name);

#if SOURCE_ENGINE >= SE_PORTAL2
	engsound->StopSound(entity, channel, name, -1);
#else
	engsound->StopSound(entity, channel, name);
#endif

	return 1;
}

static cell_t EmitSound(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr, *cl_array;
	CellRecipientFilter crf;
	unsigned int numClients;
	int client;
	IGamePlayer *pPlayer = NULL;

	pContext->LocalToPhysAddr(params[1], &cl_array);
	numClients = params[2];

	/* Client validation */
	for (unsigned int i = 0; i < numClients; i++)
	{
		client = cl_array[i];
		pPlayer = playerhelpers->GetGamePlayer(client);

		if (!pPlayer)
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		} else if (!pPlayer->IsInGame()) {
			return pContext->ThrowNativeError("Client %d is not connected", client);
		}
	}

	crf.Initialize(cl_array, numClients);

	char *sample;
	pContext->LocalToString(params[3], &sample);
	
	int entity = SoundReferenceToIndex(params[4]);
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
		for (unsigned int i = 0; i < numClients; i++)
		{
			cell_t player[1];
			player[0] = cl_array[i];
			crf.Reset();
			crf.Initialize(player, 1);
#if SOURCE_ENGINE >= SE_PORTAL2
			if (g_InSoundHook)
			{
				SH_CALL(enginesoundPatch, 
					static_cast<int (IEngineSound::*)(IRecipientFilter &, int, int, const char*, unsigned int, const char*, float, 
					soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>

					(&IEngineSound::EmitSound))
					(crf, 
					player[0], 
					channel, 
					sample, 
					-1, 
					sample, 
					vol, 
					(soundlevel_t)level, 
					0, 
					flags, 
					pitch, 
					pOrigin,
					pDir,
					pOrigVec,
					updatePos,
					soundtime,
					speakerentity);
			}
			else
			{
				engsound->EmitSound(crf, 
					player[0], 
					channel, 
					sample, 
					-1, 
					sample, 
					vol, 
					(soundlevel_t)level, 
					0, 
					flags, 
					pitch, 
					pOrigin,
					pDir,
					pOrigVec,
					updatePos,
					soundtime,
					speakerentity);
			}
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
			if (g_InSoundHook)
			{
				SH_CALL(enginesoundPatch, 
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, 
					soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>
					(&IEngineSound::EmitSound))
					(crf, 
					player[0], 
					channel, 
					sample, 
					vol, 
					(soundlevel_t)level, 
					flags, 
					pitch, 
					0, 
					pOrigin,
					pDir,
					pOrigVec,
					updatePos,
					soundtime,
					speakerentity);
			}
			else
			{
				engsound->EmitSound(crf, 
					player[0], 
					channel, 
					sample, 
					vol, 
					(soundlevel_t)level, 
					flags, 
					pitch, 
					0, 
					pOrigin,
					pDir,
					pOrigVec,
					updatePos,
					soundtime,
					speakerentity);
			}
#else
			if (g_InSoundHook)
			{
				SH_CALL(enginesoundPatch, 
					static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, 
					soundlevel_t, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>
					(&IEngineSound::EmitSound))
					(crf, 
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
			else
			{
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
#endif
		}
	} else {
#if SOURCE_ENGINE >= SE_PORTAL2
		if (g_InSoundHook)
		{
			SH_CALL(enginesoundPatch, 
				static_cast<int (IEngineSound::*)(IRecipientFilter &, int, int, const char*, unsigned int, const char*, float, 
				soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>
				(&IEngineSound::EmitSound))
				(crf, 
				entity, 
				channel, 
				sample, 
				-1, 
				sample, 
				vol, 
				(soundlevel_t)level, 
				0, 
				flags, 
				pitch, 
				pOrigin,
				pDir,
				pOrigVec,
				updatePos,
				soundtime,
				speakerentity);
		}
		else
		{
			engsound->EmitSound(crf, 
				entity, 
				channel, 
				sample, 
				-1, 
				sample, 
				vol, 
				(soundlevel_t)level, 
				0, 
				flags, 
				pitch, 
				pOrigin,
				pDir,
				pOrigVec,
				updatePos,
				soundtime,
				speakerentity);
		}
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
		if (g_InSoundHook)
		{
			SH_CALL(enginesoundPatch, 
				static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, 
				soundlevel_t, int, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>
				(&IEngineSound::EmitSound))
				(crf, 
				entity, 
				channel, 
				sample, 
				vol, 
				(soundlevel_t)level, 
				flags, 
				pitch, 
				0, 
				pOrigin,
				pDir,
				pOrigVec,
				updatePos,
				soundtime,
				speakerentity);
		}
		else
		{
			engsound->EmitSound(crf, 
				entity, 
				channel, 
				sample, 
				vol, 
				(soundlevel_t)level, 
				flags, 
				pitch, 
				0, 
				pOrigin,
				pDir,
				pOrigVec,
				updatePos,
				soundtime,
				speakerentity);
		}
#else
		if (g_InSoundHook)
		{
			SH_CALL(enginesoundPatch, 
				static_cast<void (IEngineSound::*)(IRecipientFilter &, int, int, const char*, float, 
				soundlevel_t, int, int, const Vector *, const Vector *, CUtlVector<Vector> *, bool, float, int)>
				(&IEngineSound::EmitSound))
				(crf, 
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
		else
		{
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
#endif
	}

	return 1;
}

static cell_t EmitSentence(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	CellRecipientFilter crf;
	unsigned int numClients;
	int client;
	IGamePlayer *pPlayer = NULL;

	pContext->LocalToPhysAddr(params[1], &addr);
	numClients = params[2];

	/* Client validation */
	for (unsigned int i = 0; i < numClients; i++)
	{
		client = addr[i];
		pPlayer = playerhelpers->GetGamePlayer(client);

		if (!pPlayer)
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		} else if (!pPlayer->IsInGame()) {
			return pContext->ThrowNativeError("Client %d is not connected", client);
		}
	}

	crf.Initialize(addr, numClients);

	int sentence = params[3];
	int entity = SoundReferenceToIndex(params[4]);
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
#if SOURCE_ENGINE >= SE_PORTAL2
		0, 
#endif
		flags, 
		pitch, 
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2
		0, 
#endif
		pOrigin,
		pDir,
		pOrigVec,
		updatePos,
		soundtime,
		speakerentity);

	return 1;
}

static cell_t smn_AddAmbientSoundHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[1]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);
	}

	s_SoundHooks.AddHook(AMBIENT_SOUND_HOOK, pFunc);

	return 1;
}

static cell_t smn_AddNormalSoundHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[1]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);
	}

	s_SoundHooks.AddHook(NORMAL_SOUND_HOOK, pFunc);

	return 1;
}

static cell_t smn_RemoveAmbientSoundHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[1]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);
	}

	if (!s_SoundHooks.RemoveHook(AMBIENT_SOUND_HOOK, pFunc))
	{
		return pContext->ThrowNativeError("Invalid hooked function");
	}

	return 1;
}

static cell_t smn_RemoveNormalSoundHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[1]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);
	}

	if (!s_SoundHooks.RemoveHook(NORMAL_SOUND_HOOK, pFunc))
	{
		return pContext->ThrowNativeError("Invalid hooked function");
	}

	return 1;
}

static cell_t smn_GetDistGainFromSoundLevel(IPluginContext *pContext, const cell_t *params)
{
	int decibel = params[1];
	float distance = sp_ctof(params[2]);

	return sp_ftoc(engsound->GetDistGainFromSoundLevel((soundlevel_t)decibel, distance));
}

sp_nativeinfo_t g_SoundNatives[] = 
{
	{"EmitAmbientSound",		EmitAmbientSound},
	{"EmitSentence",			EmitSentence},
	{"EmitSound",				EmitSound},
	{"FadeClientVolume",		FadeClientVolume},
	{"GetSoundDuration",		GetSoundDuration},
	{"PrefetchSound",			PrefetchSound},
	{"StopSound",				StopSound},
	{"AddAmbientSoundHook",		smn_AddAmbientSoundHook},
	{"AddNormalSoundHook",		smn_AddNormalSoundHook},
	{"RemoveAmbientSoundHook",	smn_RemoveAmbientSoundHook},
	{"RemoveNormalSoundHook",	smn_RemoveNormalSoundHook},
	{"GetDistGainFromSoundLevel", smn_GetDistGainFromSoundLevel},
	{NULL,						NULL},
};
