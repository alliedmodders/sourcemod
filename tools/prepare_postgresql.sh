#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod.
# Downloads and prepares the PostgreSQL source code for the pgsql DBI sourcemod extension.

trap "exit" INT

ismac=0
iswin=0

archive_ext=tar.gz
decomp="tar zxf"

if [ `uname` = "Darwin" ]; then
  ismac=1
elif [ `uname` != "Linux" ] && [ -n "${COMSPEC:+1}" ]; then
  iswin=1
  archive_ext=zip
  decomp=unzip
fi

sourcemodfolder="sourcemod"
if [ ! -d "sourcemod" ]; then
  sourcemodfolder="sourcemod-1.5"
  if [ ! -d "sourcemod-1.5" ]; then
    echo "Could not find a SourceMod repository; make sure you aren't running this script inside it."
    exit 1
  fi
fi

pgsqlver="9.4.0"
pgsqlmaj="9.4"
if [ $ismac -eq 0 ] && [ ! -d "postgresql-$pgsqlmaj" ]; then
  if [ `command -v wget` ]; then
    wget http://ftp.postgresql.org/pub/source/v$pgsqlver/postgresql-$pgsqlver.tar.gz -O pgsql.tar.gz
  elif [ `command -v curl` ]; then
    curl -o pgsql.tar.gz http://ftp.postgresql.org/pub/source/v$pgsqlver/postgresql-$pgsqlver.tar.gz
  else
    echo "Failed to locate wget or curl. Install one of these programs to download PostgreSQL."
    exit 1
  fi
  tar xfz pgsql.tar.gz
  mv postgresql-$pgsqlver postgresql-$pgsqlmaj
  rm pgsql.tar.gz
  
  cp $sourcemodfolder/extensions/pgsql/pg_config_paths.h postgresql-$pgsqlmaj/src/interfaces/libpq/pg_config_paths.h
  
  if [ $iswin -eq 1 ]; then
    cp postgresql-$pgsqlmaj/src/include/pg_config.h.win32 postgresql-$pgsqlmaj/src/include/pg_config.h
    cp postgresql-$pgsqlmaj/src/include/pg_config_ext.h.win32 postgresql-$pgsqlmaj/src/include/pg_config_ext.h
    cp postgresql-$pgsqlmaj/src/include/port/win32.h postgresql-$pgsqlmaj/src/include/pg_config_os.h
  else
    cd postgresql-$pgsqlmaj
	# pgsql 9.4 uses autoconf 2.69 now instead of requiring the old autoconf 2.63.
	# autoconf 2.69 is the recent one in debian packages, so this patch might not be needed anymore.
	patch configure.in < ../$sourcemodfolder/extensions/pgsql/configure_autoconf.patch
    autoconf
    ./configure --without-readline
	cd ..
  fi
fi
