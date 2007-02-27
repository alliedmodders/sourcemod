// vim: set ts=4 :
#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include "BATInterface.h"
#include <sh_list.h>
#include <sh_string.h>

using namespace SourceHook;

struct CustomFlag
{
	String name;
	AdminFlag flag;
	FlagBits bit;
};

class BatSupport : 
	public SDKExtension,
	public IMetamodListener,
	public AdminInterface,
	public IClientListener
{
public: //SDKExtension
	bool SDK_OnLoad(char *error, size_t err_max, bool late);
	void SDK_OnUnload();
	bool SDK_OnMetamodLoad(char *error, size_t err_max, bool late);
public: //IMetamodListener
	void *OnMetamodQuery(const char *iface, int *ret);
public: //AdminInterface
	bool RegisterFlag(const char *Class,const char *Flag,const char *Description);
	bool IsClient(int id);
	bool HasFlag(int id,const char *Flag);
	int GetInterfaceVersion();
	const char* GetModName();
	void AddEventListner(AdminInterfaceListner *ptr);
	void RemoveListner(AdminInterfaceListner *ptr);
public: //IClientListener
	void OnClientAuthorized(int client, const char *authstring);
private:
	List<AdminInterfaceListner *> m_hooks;
	List<CustomFlag> m_flags;
};

#endif //_INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
