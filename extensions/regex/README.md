# PCRE Build Instructions
As of writing, we're using pcre v8.44

https://ftp.pcre.org/pub/pcre/

## Windows
We build with the MinSizeForRelease VS configuration using the target architecture you want. This is how both win libs are built. The following settings were selected in the CMAKE configuration
![AenKaUByp0](https://user-images.githubusercontent.com/11095737/87211804-10db1200-c2d0-11ea-85d9-ede4ba177de1.png)

In the PCRE project, go to C/C++ configuration properties and select the Runtime Library to be Multi-threaded (`/MT`) instead of Multi-threaded DLL (`/MD`)

## Mac
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`export CFLAGS='-mmacosx-version-min=10.7'`

`./configure --enable-unicode-properties --enable-jit --disable-shared --enable-utf && make`

## Linux
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`export CFLAGS='-Wa,-mrelax-relocations=no -fPIC'`

`./configure --enable-unicode-properties --enable-jit --disable-shared --enable-utf && make`
