#include "sm_autonatives.h"
#include "PluginSys.h"

void CoreNativesToAdd::OnSourceModAllInitialized()
{
	g_PluginSys.RegisterNativesFromCore(m_NativeList);
}
