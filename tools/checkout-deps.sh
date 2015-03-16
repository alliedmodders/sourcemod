#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

trap "exit" INT

ismac=0
iswin=0
isci=0

archive_ext=tar.gz
decomp="tar zxf"

if [ `uname` = "Darwin" ]; then
  ismac=1
elif [ `uname` != "Linux" ] && [ -n "${COMSPEC:+1}" ]; then
  iswin=1
  archive_ext=zip
  decomp=unzip
fi

if [ -n "${TRAVIS+1}" -o -n "${CONTINUOUS_INTEGRATION+1}" ]; then
 isci = 1
fi


if [ ! -d "sourcemod" ]; then
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
  free -m
  du -sh /tmp/*
  ps aux --sort -rss
  if [ ! -d "$name" ]; then
    git clone --shared $repo -b $branch $name
    if [ -n "$origin" ]; then
      cd $name
      git remote rm origin
      git remote add origin $origin
      cd ..
    fi
  else
    cd $name
    git checkout $branch
    git pull origin $branch
    cd ..
  fi
}

name=mmsource-1.10
branch=1.10-dev
repo="https://github.com/alliedmodders/metamod-source"
origin=
checkout

if [ $isci -eq 0 ]; then
  sdks=( csgo hl2dm nucleardawn l4d2 dods l4d css tf2 insurgency sdk2013 dota )

  if [ $ismac -eq 0 ]; then
    # Add these SDKs for Windows or Linux
    sdks+=( orangebox blade episode1 )

    # Add more SDKs for Windows only
    if [ $iswin -eq 1 ]; then
      sdks+=( darkm swarm bgt eye contagion )
    fi
  fi
else
  sdks=( episode1,tf2,l4d2,csgo,dota )
fi

# Check out a local copy as a proxy.
if [ ! -d "hl2sdk-proxy-repo" ]; then
  git clone --shared --mirror https://github.com/alliedmodders/hl2sdk hl2sdk-proxy-repo
else
  cd hl2sdk-proxy-repo
  git fetch
  cd ..
fi

for sdk in "${sdks[@]}"
do
  repo=hl2sdk-proxy-repo
  origin="https://github.com/alliedmodders/hl2sdk"
  name=hl2sdk-$sdk
  branch=$sdk
  checkout
done

`python -c "import ambuild2"`
if [ $? -eq 1 ]; then
  repo="https://github.com/alliedmodders/ambuild"
  origin=
  branch=master
  name=ambuild
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
