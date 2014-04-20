#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

trap "exit" INT

ismac=0
iswin=0

archive_ext=tar.gz
decomp="tar zxvf"

if [ `uname` = "Darwin" ]; then
  ismac=1
elif [ `uname` != "Linux" ] && [ -n "${COMSPEC:+1}" ]; then
  iswin=1
  archive_ext=zip
  decomp=unzip
fi

if [ ! -d "sourcemod-central" ]; then
  if [ ! -d "sourcemod-1.5" ]; then
    echo "Could not find a SourceMod repository; make sure you aren't running this script inside it."
    exit 1
  fi
fi

if [ $ismac -eq 1 ]; then
  mysqlver=mysql-5.5.28-osx10.5-x86
  mysqlurl=http://cdn.mysql.com/archives/mysql-5.5/$mysqlver.$archive_ext
elif [ $iswin -eq 1 ]; then
  mysqlver=mysql-noinstall-5.0.24a-win32
  mysqlurl=http://cdn.mysql.com/archives/mysql-5.0/$mysqlver.$archive_ext
  # The folder in the zip archive does not contain the substring "-noinstall", so strip it
  mysqlver=${mysqlver/-noinstall}
else
  mysqlver=mysql-5.6.15-linux-glibc2.5-i686
  mysqlurl=http://cdn.mysql.com/archives/mysql-5.6/$mysqlver.$archive_ext
fi

if [ ! -d "mysql-5.0" ]; then
  if [ `command -v wget` ]; then
    wget $mysqlurl -O mysql.$archive_ext
  elif [ `command -v curl` ]; then
    curl -o mysql.$archive_ext $mysqlurl
  else
    echo "Failed to locate wget or curl. Install one of these programs to download MySQL."
    exit 1
  fi
  $decomp mysql.$archive_ext
  mv $mysqlver mysql-5.0
  rm mysql.$archive_ext
fi

checkout ()
{
  if [ ! -d "$name" ]; then
    hg clone https://hg.alliedmods.net/$path/$name
  else
    cd $name
    hg pull -u
    cd ..
  fi
}

name=mmsource-1.10
path=releases
checkout

sdks=( csgo hl2dm nd l4d2 dods l4d css tf2 insurgency 2013 dota )

if [ $ismac -eq 0 ]; then
  # Checkout original HL2 SDK on Windows or Linux
  name=hl2sdk
  path=hl2sdks
  checkout

  # Add these SDKs for Windows or Linux
  sdks+=( ob blade )

  # Add more SDKs for Windows only
  if [ $iswin -eq 1 ]; then
    sdks+=( darkm swarm bgt eye )
  fi
fi

for sdk in "${sdks[@]}"
do
  name=hl2sdk-$sdk
  path=hl2sdks
  checkout
done

`python -c "import ambuild2"`
if [ $? -eq 1 ]; then
  name=
  path=ambuild
  checkout

  cd ambuild
  if [ $iswin -eq 1 ]; then
    python setup.py install
  else
    python setup.py build
    echo "About to install AMBuild - press Ctrl+C to abort, otherwise enter your password for sudo."
    sudo python setup.py install
  fi
fi
