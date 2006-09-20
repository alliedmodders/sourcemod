#ifndef _INCLUDE_SOURCEPAWN_VM_BASE_H_
#define _INCLUDE_SOURCEPAWN_VM_BASE_H_

#include <sp_vm_api.h>

#if defined WIN32
#define EXPORT_LINK		extern "C" __declspec(dllexport)
#else if defined __GNUC__
#define EXPORT_LINK		extern "C" __attribute__((visibility("default")))
#endif

typedef SourcePawn::IVirtualMachine *(*SP_GETVM_FUNC)(SourcePawn::ISourcePawnEngine *);

#endif //_INCLUDE_SOURCEPAWN_VM_BASE_H_
