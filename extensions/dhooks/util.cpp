#include "util.h"

void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset)
{
#ifdef  WIN32
	if (type == HookParamType_Object)
		return (void *)((intptr_t)params + offset);
#elif POSIX
	if (type == HookParamType_Object && !(flags & PASSFLAG_ODTOR)) //Objects are passed by rrefrence if they contain destructors.
		return (void *)((intptr_t)params + offset);
#endif
	return *(void **)((intptr_t)params + offset);

}

size_t GetParamOffset(HookParamsStruct *paramStruct, unsigned int index)
{
	size_t offset = 0;
	for (unsigned int i = 0; i < index; i++)
	{
#ifndef WIN32
		if (paramStruct->dg->params.at(i).type == HookParamType_Object && (paramStruct->dg->params.at(i).flags & PASSFLAG_ODTOR)) //Passed by refrence
		{
			offset += sizeof(void *);
			continue;
		}
#endif
		offset += paramStruct->dg->params.at(i).size;
	}
	return offset;
}

size_t GetParamTypeSize(HookParamType type)
{
	return sizeof(void *);
}

size_t GetParamsSize(DHooksCallback *dg)//Get the full size, this is for creating the STACK.
{
	size_t res = 0;

	for (int i = dg->params.size() - 1; i >= 0; i--)
	{
		res += dg->params.at(i).size;
	}

	return res;
}
