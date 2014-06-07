#ifndef _INCLUDE_V8_DLL_H_
#define _INCLUDE_V8_DLL_H_

#if defined WIN32
#define EXPORTFUNC	extern "C" __declspec(dllexport)
#elif defined __GNUC__
#if __GNUC__ >= 3
#define EXPORTFUNC extern "C" __attribute__((visibility("default")))
#else
#define EXPORTFUNC extern "C"
#endif //__GNUC__ >= 3
#endif //defined __GNUC__

#endif //_INCLUDE_V8_DLL_H_
