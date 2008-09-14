/**
 * vim: set ts=4 :
 * ======================================================
 * Metamod:Source Stub Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This stub plugin is public domain.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_STUB_UTIL_FUNCTIONS_H_
#define _INCLUDE_STUB_UTIL_FUNCTIONS_H_

#include <stddef.h>

/**
 * This is a platform-safe function which fixes weird idiosyncracies 
 * in the null-termination and return value of snprintf().  It guarantees 
 * the terminator on overflow cases, and never returns -1 or a value 
 * not equal to the number of non-terminating bytes written.
 */
size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

#endif //_INCLUDE_STUB_UTIL_FUNCTIONS_H_

