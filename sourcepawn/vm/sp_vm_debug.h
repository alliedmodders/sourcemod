#ifndef _INCLUDE_SOURCEPAWN_VM_DEBUG_H
#define _INCLUDE_SOURCEPAWN_VM_DEBUG_H

/*****************
 ** Note that all functions return a non-zero error code on failure
 *   unless otherwise noted.
 * All input pointers must be valid unless otherwise noted as optional.
 * All output pointers on failure are undefined.
 * All local address are guaranteed to be positive.  However, they are stored
 *  as signed integers, because they must logically fit inside a cell.
 */

/**
 * Given a code pointer, finds the file it is associated with.
 * 
 * @param ctx		Context pointer.
 * @param addr		Code address offset.
 * @param filename	Pointer to store filename pointer in.
 */
int SP_DbgLookupFile(sp_context_t *ctx, ucell_t addr, const char **filename);

/**
 * Given a code pointer, finds the function it is associated with.
 *
 * @param ctx		Context pointer.
 * @param addr		Code address offset.
 * @param name		Pointer to store function name pointer in.
 */
int SP_DbgLookupFunction(sp_context_t *ctx, ucell_t addr, const char **name);

/**
 * Given a code pointer, finds the line it is associated with.
 *
 * @param ctx		Context pointer.
 * @param addr		Code address offset.
 * @param line		Pointer to store line number in.
 */
int SP_DbgLookupLine(sp_context_t *ctx, ucell_t addr, uint32_t *line);

/**
 * Installs a debug break and returns the old one, if any.
 *
 * @param ctx		Context pointer.
 * @param newpfn	New function pointer.
 * @param oldpfn	Pointer to retrieve old function pointer.
 */
int SP_DbgInstallBreak(sp_context_t *ctx, 
					   SPVM_DEBUGBREAK newpfn, 
					   SPVM_DEBUGBREAK *oldpfn);


#endif //_INCLUDE_SOURCEPAWN_VM_DEBUG_H
