#include "dll_exports.h"
#include "V8Manager.h"


SMV8::Manager g_manager;

EXPORTFUNC SMV8::IManager *GetV8Manager()
{
	return &g_manager;
}