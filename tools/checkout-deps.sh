#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

trap "exit" INT

ismac=0
if [ `uname` = "Darwin" ]; then
  ismac=1
fi

if [ ! -d "sourcemod-central" ]; then
  if [ ! -d "sourcemod-1.5" ]; then
    mkdir -p sm-dependencies
    cd sm-dependencies
  fi
fi

if [ $ismac -eq 1 ]; then
  mysqlver=mysql-5.5.28-osx10.5-x86
  mysqlurl=http://cdn.mysql.com/archives/mysql-5.5/$mysqlver.tar.gz
else
  mysqlver=mysql-5.6.15-linux-glibc2.5-i686
  mysqlurl=http://cdn.mysql.com/archives/mysql-5.6/$mysqlver.tar.gz
fi

if [ ! -d "mysql-5.0" ]; then
  if [ `command -v wget` ]; then
    wget $mysqlurl -O mysql.tar.gz
  elif [ `command -v curl` ]; then
    curl -o mysql.tar.gz $mysqlurl
  else
    echo "Failed to locate wget or curl. Install one of these programs to download MySQL."
    exit 1
  fi
  tar zxvf mysql.tar.gz
  mv $mysqlver mysql-5.0
  rm mysql.tar.gz
fi

checkout ()
{
  if [ ! -d "$name" ]; then
    hg clone http://hg.alliedmods.net/$path/$name
  else
    cd $name
    hg pull -u
    cd ..
  fi
}

name=mmsource-1.10
path=releases
checkout

sdks=( csgo hl2dm nd l4d2 dods l4d css tf2 insurgency 2013 )

if [ $ismac -eq 0 ]; then
  sdks+=( ob blade )

  name=hl2sdk
  path=hl2sdks
  checkout
fi

for sdk in "${sdks[@]}"
do
  name=hl2sdk-$sdk
  path=hl2sdks
  checkout
done

