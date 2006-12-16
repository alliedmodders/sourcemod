#include "sourcemm_api.h"
#include <convar.h>
#include "sourcemod.h"
#include "PluginSys.h"

using namespace SourceMod;

class ConVarAccessor : 
	public IConCommandBaseAccessor,
	public SMGlobalClass
{
public: // IConCommandBaseAccessor
	virtual bool RegisterConCommandBase(ConCommandBase *pCommand);
public: // SMGlobalClass
	virtual void OnSourceModStartup(bool late);
};