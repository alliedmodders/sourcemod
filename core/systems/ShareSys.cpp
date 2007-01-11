#include "ShareSys.h"
#include "HandleSys.h"

ShareSystem g_ShareSys;

ShareSystem::ShareSystem()
{
	m_IdentRoot.ident = 0;
	m_TypeRoot = 0;
	m_IfaceType = 0;
	m_CoreType = 0;
}

IdentityToken_t *ShareSystem::CreateCoreIdentity()
{
	if (!m_CoreType)
	{
		m_CoreType = CreateIdentType("CORE");
	}

	return CreateIdentity(m_CoreType);
}

void ShareSystem::OnSourceModStartup(bool late)
{
	TypeAccess sec;

	g_HandleSys.InitAccessDefaults(&sec, NULL);
	sec.ident = GetIdentRoot();

	m_TypeRoot = g_HandleSys.CreateType("Identity", this, 0, &sec, NULL, NULL, NULL);
	m_IfaceType = g_HandleSys.CreateType("Interface", this, 0, NULL, NULL, GetIdentRoot(), NULL);

	/* Initialize our static identity handle */
	m_IdentRoot.ident = g_HandleSys.CreateHandle(m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);
}

void ShareSystem::OnSourceModShutdown()
{
	if (m_CoreType)
	{
		g_HandleSys.RemoveType(m_CoreType, GetIdentRoot());
	}

	g_HandleSys.RemoveType(m_IfaceType, GetIdentRoot());
	g_HandleSys.RemoveType(m_TypeRoot, GetIdentRoot());
}

IdentityType_t ShareSystem::FindIdentType(const char *name)
{
	HandleType_t type;

	if (g_HandleSys.FindHandleType(name, &type))
	{
		if (g_HandleSys.TypeCheck(type, m_TypeRoot))
		{
			return type;
		}
	}

	return 0;
}

IdentityType_t ShareSystem::CreateIdentType(const char *name)
{
	if (!m_TypeRoot)
	{
		return 0;
	}

	return g_HandleSys.CreateType(name, this, m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);
}

void ShareSystem::OnHandleDestroy(HandleType_t type, void *object)
{
	/* THIS WILL NEVER BE CALLED FOR ANYTHING WITH THE IDENTITY TYPE */
}

IdentityToken_t *ShareSystem::CreateIdentity(IdentityType_t type)
{
	if (!m_TypeRoot)
	{
		return 0;
	}

	/* :TODO: Cache? */
	IdentityToken_t *pToken = new IdentityToken_t;
	pToken->ident = g_HandleSys.CreateHandleEx(type, NULL, GetIdentRoot(), GetIdentRoot(), NULL, true);

	return pToken;
}

bool ShareSystem::AddInterface(SMInterface *iface, IdentityToken_t *token)
{
	if (!iface)
	{
		return false;
	}

	IfaceInfo info;

	info.iface = iface;
	info.token = token;
	
	if (token)
	{
		/* If we're an external object, we have to do this */
		info.handle = g_HandleSys.CreateHandle(m_IfaceType, iface, token, GetIdentRoot(), NULL);
	} else {
		info.handle = 0;
	}

	m_Interfaces.push_back(info);

	return true;
}

bool ShareSystem::RequestInterface(const char *iface_name, 
								   unsigned int iface_vers, 
								   IdentityToken_t *token, 
								   SMInterface **pIface)
{
	/* If Some yahoo.... SOME HOOLIGAN... some NO GOOD DIRTY 
	 * HORRIBLE PERSON passed in a token that we don't recognize....
	 * <b>Punish them.</b>
	 */
	HandleSecurity sec;

	sec.pIdentity = GetIdentRoot();
	sec.pOwner = NULL;

	if (!g_HandleSys.ReadHandle(token->ident, m_TypeRoot, &sec, NULL))
	{
		return false;
	}

	/* See if the interface exists */
	List<IfaceInfo>::iterator iter;
	SMInterface *iface;
	IdentityToken_t *iface_owner;
	Handle_t iface_handle;
	bool found = false;
	for (iter=m_Interfaces.begin(); iter!=m_Interfaces.end(); iter++)
	{
		IfaceInfo &info = (*iter);
		iface = info.iface;
		if (strcmp(iface->GetInterfaceName(), iface_name) == 0)
		{
			if (iface->GetInterfaceVersion() == iface_vers
				|| iface->IsVersionCompatible(iface_vers))
			{
				iface_owner = info.token;
				iface_handle = info.handle;
				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		return false;
	}

	/* If something external owns this, we need to track it. */
	if (iface_owner)
	{
		Handle_t newhandle;
		if (g_HandleSys.CloneHandle(iface_handle, &newhandle, token, &sec)
			!= HandleError_None)
		{
			return false;
		}
		/** 
		 * Now we can deny module loads based on dependencies.
		 */
	}

	/* :TODO: finish */

	return NULL;
}

void ShareSystem::AddNatives(IdentityToken_t *token, const sp_nativeinfo_t *natives[])
{
	/* :TODO: implement */
}

void ShareSystem::DestroyIdentity(IdentityToken_t *identity)
{
	HandleSecurity sec;

	sec.pOwner = GetIdentRoot();
	sec.pIdentity = GetIdentRoot();

	g_HandleSys.FreeHandle(identity->ident, &sec);
	delete identity;
}

void ShareSystem::DestroyIdentType(IdentityType_t type)
{
	g_HandleSys.RemoveType(type, GetIdentRoot());
}

