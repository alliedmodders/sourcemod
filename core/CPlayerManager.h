#ifndef _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
#define _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_

#include "sourcemod.h"
#include <eiface.h>
#include "sourcemm_api.h"
#include <IForwardSys.h>

class CPlayerManager : public SMGlobalClass
{
public: //SMGlobalClass
	virtual void OnSourceModAllInitialized();
	virtual void OnSourceModShutdown();
public:
	bool OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	void OnClientPutInServer(edict_t *pEntity, char const *playername);
	void OnClientDisconnect(edict_t *pEntity);
	void OnClientDisconnect_Post(edict_t *pEntity);
	void OnClientAuthorized(); //:TODO: any args needed?
	void OnClientCommand(edict_t *pEntity);
private:
	IForward *m_clconnect;
	IForward *m_cldisconnect;
	IForward *m_cldisconnect_post;
	IForward *m_clputinserver;
	IForward *m_clcommand;
};

#endif //_INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_