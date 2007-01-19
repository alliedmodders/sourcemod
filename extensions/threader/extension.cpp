#include "extension.h"
#include "thread/ThreadSupport.h"

Sample g_Sample;
MainThreader g_Threader;

SMEXT_LINK(&g_Sample);

bool Sample::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	g_pShareSys->AddInterface(myself, &g_Threader);

	return true;
}
