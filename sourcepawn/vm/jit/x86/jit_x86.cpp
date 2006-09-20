#include <string.h>
#include <stdlib.h>
#include "jit_x86.h"

const char *JITX86::GetVMName()
{
	return "JIT (x86)";
}

ICompilation *JITX86::StartCompilation(sp_plugin_t *plugin)
{
	CompData *data = new CompData;

	data->plugin = plugin;

	return data;
}

void JITX86::AbortCompilation(ICompilation *co)
{
	delete (CompData *)co;
}

bool JITX86::SetCompilationOption(ICompilation *co, const char *key, const char *val)
{
	CompData *data = (CompData *)co;

	if (strcmp(key, "debug") == 0)
	{
		data->debug = (atoi(val) == 1);
		if (data->debug && !(data->plugin->flags & SP_FLAG_DEBUG))
		{
			data->debug = false;
			return false;
		}
		return true;
	}

	return false;
}
