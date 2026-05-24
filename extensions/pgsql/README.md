# PostgreSQL Static Libraries

This extension bundles PostgreSQL 18.4 static libraries built by:

https://github.com/ProjectSky/build-postgresql-library

Use that project to rebuild or update the bundled libraries. It contains the
current PostgreSQL version, Linux configure flags, Windows Meson flags, and
release packaging details.

## Manual Build

The current build uses PostgreSQL `18.4`.

1. Install the build dependencies.

   Linux requires `build-essential gcc-multilib g++-multilib`.

   Windows requires an MSVC developer shell, `winflexbison3`, `meson`, and
   `ninja`.

2. Download and unpack [postgresql-18.4.tar.gz](https://ftp.postgresql.org/pub/latest/postgresql-18.4.tar.gz)

3. For Linux, create a separate build directory for each architecture and
   configure with `--without-readline --without-icu --without-zlib`.

   Use `CFLAGS="-fPIC -m32" LDFLAGS="-m32"` for `lib_linux`, or
   `CFLAGS="-fPIC -m64" LDFLAGS="-m64"` for `lib_linux64`.

4. Build the Linux static libraries:

```sh
make -C src/backend generated-headers
make -C src/port libpgport_shlib.a
make -C src/common libpgcommon_shlib.a
make -C src/interfaces/libpq libpq.a SHLIB_PREREQS=
```

5. For Windows, configure from an MSVC developer shell:

```bat
python -m mesonbuild.mesonmain setup build-win64 . --backend=ninja -Dreadline=disabled -Dicu=disabled -Dlibcurl=disabled -Dzlib=disabled -Dssl=none -Db_vscrt=mt
```

6. Build the Windows static libraries:

```bat
python -m ninja -C build-win64 src/interfaces/libpq/libpq.a src/common/libpgcommon_shlib.a src/port/libpgport_shlib.a
```

   Use `build-win32` from an x86 MSVC shell for `lib_win`.

7. Copy the three static libraries plus `libpq-fe.h` and `postgres_ext.h` into
   the matching `lib_*` directory. Windows packages rename the `.a` outputs to
   `.lib`.

## Bundled Files

Linux:

- `lib_linux/libpq.a`
- `lib_linux/libpgcommon_shlib.a`
- `lib_linux/libpgport_shlib.a`
- `lib_linux64/libpq.a`
- `lib_linux64/libpgcommon_shlib.a`
- `lib_linux64/libpgport_shlib.a`

Windows:

- `lib_win/libpq.lib`
- `lib_win/libpgcommon_shlib.lib`
- `lib_win/libpgport_shlib.lib`
- `lib_win64/libpq.lib`
- `lib_win64/libpgcommon_shlib.lib`
- `lib_win64/libpgport_shlib.lib`

Headers:

- `libpq-fe.h`
- `postgres_ext.h`

macOS pgsql builds are not currently supported.
