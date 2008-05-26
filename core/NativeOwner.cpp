#include "NativeOwner.h"
#include "ShareSys.h"
#include "PluginSys.h"

void CNativeOwner::SetMarkSerial(unsigned int serial)
{
	m_nMarkSerial = serial;
}

unsigned int CNativeOwner::GetMarkSerial()
{
	return m_nMarkSerial;
}

void CNativeOwner::AddDependent(CPlugin *pPlugin)
{
	m_Dependents.push_back(pPlugin);
}

void CNativeOwner::AddWeakRef(const WeakNative & ref)
{
	m_WeakRefs.push_back(ref);
}

void CNativeOwner::AddNatives(const sp_nativeinfo_t *natives)
{
	NativeEntry *pEntry;

	for (unsigned int i = 0; natives[i].func != NULL && natives[i].name != NULL; i++)
	{
		if ((pEntry = g_ShareSys.AddNativeToCache(this, &natives[i])) == NULL)
		{
			continue;
		}

		m_Natives.push_back(pEntry);
	}
}

void CNativeOwner::PropogateMarkSerial(unsigned int serial)
{
	CNativeOwner *pOwner;
	List<CPlugin *>::iterator iter;

	for (iter = m_Dependents.begin();
		 iter != m_Dependents.end();
		 iter++)
	{
		pOwner = (*iter);
		pOwner->SetMarkSerial(serial);
	}
}

void CNativeOwner::UnbindWeakRef(const WeakNative & ref)
{
	sp_native_t *native;
	IPluginContext *pContext;

	pContext = ref.pl->GetBaseContext();
	if ((pContext->GetNativeByIndex(ref.idx, &native)) == SP_ERROR_NONE)
	{
		/* If there is no reference, the native must be unbound */
		if (ref.entry == NULL)
		{
			native->status = SP_NATIVE_UNBOUND;
			native->pfn = NULL;
		}
		/* If we've cached a reference, it's a core native we can 
		 * rebind back to its original (this was a replacement).
		 */
		else
		{
			native->pfn = ref.entry->func;
		}
	}
}

void CNativeOwner::DropEverything()
{
	NativeEntry *pEntry;
	List<WeakNative>::iterator iter;
	List<NativeEntry *>::iterator ntv_iter;

	/* Unbind and remove all weak references to us */
	iter = m_WeakRefs.begin();
	while (iter != m_WeakRefs.end())
	{
		UnbindWeakRef((*iter));
		iter = m_WeakRefs.erase(iter);
	}

	/* Unmark our replacement natives */
	ntv_iter = m_ReplacedNatives.begin();
	while (ntv_iter != m_ReplacedNatives.end())
	{
		pEntry = (*ntv_iter);
		pEntry->replacement.func = NULL;
		pEntry->replacement.owner = NULL;
		ntv_iter = m_ReplacedNatives.erase(ntv_iter);
	}

	/* Strip all of our natives from the cache */
	ntv_iter = m_Natives.begin();
	while (ntv_iter != m_Natives.end())
	{
		g_ShareSys.ClearNativeFromCache(this, (*ntv_iter)->name);
		ntv_iter = m_Natives.erase(ntv_iter);
	}
}

void CNativeOwner::DropWeakRefsTo(CPlugin *pPlugin)
{
	List<WeakNative>::iterator iter;

	iter = m_WeakRefs.begin();
	while (iter != m_WeakRefs.end())
	{
		WeakNative & ref = (*iter);

		if (ref.pl == pPlugin)
		{
			iter = m_WeakRefs.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void CNativeOwner::DropRefsTo(CPlugin *pPlugin)
{
	m_Dependents.remove(pPlugin);
	DropWeakRefsTo(pPlugin);
}

void CNativeOwner::AddReplacedNative(NativeEntry *pEntry)
{
	m_ReplacedNatives.push_back(pEntry);
}
