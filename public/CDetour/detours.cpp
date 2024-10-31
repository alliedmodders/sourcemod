#include "detours.h"
#include <cstdio>

ISourcePawnEngine *CDetourManager::spengine = NULL;
IGameConfig *CDetourManager::gameconf = NULL;

void CDetourManager::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	CDetourManager::spengine = spengine;
	CDetourManager::gameconf = gameconf;
}

CDetour *CDetourManager::CreateDetour(void *callbackfunction, void **trampoline, const char *signame)
{
	void* pAddress;
	if (!gameconf->GetMemSig(signame, &pAddress))
	{
		g_pSM->LogError(myself, "Signature for %s not found in gamedata", signame);
		return NULL;
	}

	if (!pAddress)
	{
		g_pSM->LogError(myself, "Sigscan for %s failed", signame);
		return NULL;
	}

	return CreateDetour(callbackfunction, trampoline, pAddress);
}

CDetour *CDetourManager::CreateDetour(void *callbackFunction, void **trampoline, void *pAddress)
{
	CDetour* detour = new CDetour(callbackFunction, trampoline, pAddress);

	auto result = safetyhook::InlineHook::create(pAddress, callbackFunction, safetyhook::InlineHook::Flags::StartDisabled);
	if(result)
	{
		detour->m_hook = std::move(result.value());
		*trampoline = detour->m_hook.original<void*>();
	}
	else
	{
		auto err = result.error();
		switch(err.type)
		{
			case safetyhook::InlineHook::Error::BAD_ALLOCATION:
				if(err.allocator_error == safetyhook::Allocator::Error::BAD_VIRTUAL_ALLOC)
				{
					g_pSM->LogError(myself, "BAD_VIRTUAL_ALLOC hook %p", pAddress);
				}
				else if(err.allocator_error == safetyhook::Allocator::Error::NO_MEMORY_IN_RANGE)
				{
					g_pSM->LogError(myself, "NO_MEMORY_IN_RANGE hook %p", pAddress);
				}
				else
				{
					g_pSM->LogError(myself, "BAD_ALLOCATION hook %p errnum %i", pAddress, err.allocator_error);
				}
				break;
			case safetyhook::InlineHook::Error::FAILED_TO_DECODE_INSTRUCTION:
				g_pSM->LogError(myself, "FAILED_TO_DECODE_INSTRUCTION hook %p ip %p", pAddress, err.ip);
				break;
			case safetyhook::InlineHook::Error::SHORT_JUMP_IN_TRAMPOLINE:
				g_pSM->LogError(myself, "SHORT_JUMP_IN_TRAMPOLINE hook %p ip %p", pAddress, err.ip);
				break;
			case safetyhook::InlineHook::Error::IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE:
				g_pSM->LogError(myself, "IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE hook %p ip %p", pAddress, err.ip);
				break;
			case safetyhook::InlineHook::Error::UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE:
				g_pSM->LogError(myself, "UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE hook %p ip %p", pAddress, err.ip);
				break;
			case safetyhook::InlineHook::Error::FAILED_TO_UNPROTECT:
				g_pSM->LogError(myself, "FAILED_TO_UNPROTECT hook %p ip %p", pAddress, err.ip);
				break;
			case safetyhook::InlineHook::Error::NOT_ENOUGH_SPACE:
				g_pSM->LogError(myself, "NOT_ENOUGH_SPACE hook %p ip %p", pAddress, err.ip);
				break;
			default:
				g_pSM->LogError(myself, "Unknown error %i hook %p ip %p", err.type, pAddress, err.ip);
				break;
		}
		
		delete detour;
		return NULL;		
	}

	return detour;
}

CDetour::CDetour(void* callbackFunction, void **trampoline, void *pAddress)
{
}

bool CDetour::IsEnabled()
{
	return m_hook.enabled();
}

void CDetour::EnableDetour()
{
	m_hook.enable();
}

void CDetour::DisableDetour()
{
	m_hook.disable();
}

void CDetour::Destroy()
{
	delete this;
}