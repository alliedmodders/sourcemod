#include <sp_vm_api.h>
#include "jit_x86.h"
#include "dll_exports.h"

SourcePawn::ISourcePawnEngine *engine = NULL;
JITX86 jit;

EXPORTFUNC void GiveEnginePointer(SourcePawn::ISourcePawnEngine *engine_p)
{
	engine = engine_p;
}

EXPORTFUNC unsigned int GetExportCount()
{
	return 1;
}

EXPORTFUNC SourcePawn::IVirtualMachine *GetExport(unsigned int exportnum)
{
	/* Don't return anything if we're not initialized yet */
	if (!engine)
	{
		return NULL;
	}

	/* We only have one export - 0 */
	if (exportnum)
	{
		return NULL;
	}

	return &jit;
}
