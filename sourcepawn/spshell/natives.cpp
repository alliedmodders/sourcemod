#include <sp_vm_api.h>
#include "natives.h"

using namespace SourcePawn;

static cell_t ReturnFour(IPluginContext *pContext, const cell_t *params)
{
	return 4;
}

sp_nativeinfo_t g_Natives[] = 
{
	{"ReturnFour",			ReturnFour},
	{NULL,					NULL}
};
