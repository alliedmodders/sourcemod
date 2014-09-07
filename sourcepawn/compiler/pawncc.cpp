#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include "memfile.h"
#include "osdefs.h"
#if defined LINUX || defined DARWIN
#include <unistd.h>
#elif defined WIN32
#include <io.h>
#endif
#include "sc.h"

int main(int argc, char *argv[])
{
  return pc_compile(argc,argv);
}

#if defined __linux__ || defined __APPLE__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif
