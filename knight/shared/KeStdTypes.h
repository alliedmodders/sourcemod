#ifndef _INCLUDE_KNIGHT_KE_STANDARD_TYPES_H_
#define _INCLUDE_KNIGHT_KE_STANDARD_TYPES_H_

#include <KePlatform.h>
#include <stddef.h>

#if defined KE_PLATFORM_WINDOWS

typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else

#include <stdint.h>

#endif

#endif //_INCLUDE_KNIGHT_KE_STANDARD_TYPES_H_
