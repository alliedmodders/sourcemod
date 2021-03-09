# PostgreSQL Build Instructions
As of writing, we're using PostgreSQL v9.6.9

https://www.postgresql.org/ftp/source/

After building, libpq can be found in src/interfaces/libpq/

## Windows
https://www.postgresql.org/docs/9.6/install-windows-libpq.html

Change `src/interfaces/win32.mak` line 35 from `OPT=/O2 /MD` to `OPT=/O2 /MT`. Library will be in `interfaces\libpq\Release\libpq.lib`.
You have to delete the interfaces\libpq\Release folder between x86 and x86_64 builds.

## Mac
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`CFLAGS='-mmacosx-version-min=10.7' ./configure && make`

## Linux
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`CFLAGS='-fPIC' ./configure --without-readline && make`
