# PostgreSQL Build Instructions
As of writing, we're using PostgreSQL v9.6.9

https://www.postgresql.org/ftp/source/

After building, libpq can be found in src/interfaces/libpq/

## Windows
https://www.postgresql.org/docs/9.6/install-windows-libpq.html

## Mac
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`CFLAGS='-mmacosx-version-min=10.7' ./configure && make`

## Linux
For x86 or x86_64 add -m32 or -m64 to `CFLAGS`

`CFLAGS='-fPIC' ./configure --without-readline && make`
