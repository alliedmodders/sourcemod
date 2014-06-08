#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

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

if [ ! -d "sourcemod" ]; then
  if [ ! -d "sourcemod-1.5" ]; then
    echo "Could not find a SourceMod repository; make sure you aren't running this script inside it."
    exit 1
  fi
fi

getmysql ()
{
  if [ ! -d $mysqlfolder ]; then
    if [ `command -v wget` ]; then
      wget $mysqlurl -O $mysqlfolder.$archive_ext
    elif [ `command -v curl` ]; then
      curl -o $mysqlfolder.$archive_ext $mysqlurl
    else
      echo "Failed to locate wget or curl. Install one of these programs to download MySQL."
      exit 1
    fi
    $decomp $mysqlfolder.$archive_ext
    mv $mysqlver $mysqlfolder
    rm $mysqlfolder.$archive_ext
  fi
}

# 32-bit MySQL
mysqlfolder=mysql-5.5
if [ $ismac -eq 1 ]; then
  mysqlver=mysql-5.5.28-osx10.5-x86
  mysqlurl=https://cdn.mysql.com/archives/mysql-5.5/$mysqlver.$archive_ext
elif [ $iswin -eq 1 ]; then
  mysqlver=mysql-5.5.54-win32
  mysqlurl=https://cdn.mysql.com/archives/mysql-5.5/$mysqlver.$archive_ext
  # The folder in the zip archive does not contain the substring "-noinstall", so strip it
  mysqlver=${mysqlver/-noinstall}
else
  mysqlver=mysql-5.6.15-linux-glibc2.5-i686
  mysqlurl=https://cdn.mysql.com/archives/mysql-5.6/$mysqlver.$archive_ext
fi
getmysql

# 64-bit MySQL
mysqlfolder=mysql-5.5-x86_64
if [ $ismac -eq 1 ]; then
  mysqlver=mysql-5.5.28-osx10.5-x86_64
  mysqlurl=https://cdn.mysql.com/archives/mysql-5.5/$mysqlver.$archive_ext
elif [ $iswin -eq 0 ]; then
  mysqlver=mysql-5.6.15-linux-glibc2.5-x86_64
  mysqlurl=https://cdn.mysql.com/archives/mysql-5.6/$mysqlver.$archive_ext
fi
getmysql

if [ $ismac -eq 0 ] && [ $iswin -eq 0 ] && [ ! -d "postgresql-9.3" ]; then
  if [ `command -v wget` ]; then
    wget http://ftp.postgresql.org/pub/source/v9.3.4/postgresql-9.3.4.tar.gz -O pgsql.tar.gz
  elif [ `command -v curl` ]; then
    curl -o pgsql.tar.gz http://ftp.postgresql.org/pub/source/v9.3.4/postgresql-9.3.4.tar.gz
  else
    echo "Failed to locate wget or curl. Install one of these programs to download PostgreSQL."
    exit 1
  fi
  $decomp pgsql.tar.gz
  mv postgresql-9.3.4 postgresql-9.3
  rm pgsql.tar.gz
fi

checkout ()
{
  if [ ! -d "$name" ]; then
    git clone $repo -b $branch $name
    if [ -n "$origin" ]; then
      cd $name
      git remote set-url origin $origin
      cd ..
    fi
  else
    cd $name
    if [ -n "$origin" ]; then
      git remote set-url origin ../$repo
    fi
    git checkout $branch
    git pull origin $branch
    if [ -n "$origin" ]; then
      git remote set-url origin $origin
    fi
    cd ..
  fi
}

name=mmsource-1.10
branch=1.10-dev
repo="https://github.com/alliedmodders/metamod-source"
origin=
checkout

sdks=( csgo hl2dm nucleardawn l4d2 dods l4d css tf2 insurgency sdk2013 doi )

if [ $ismac -eq 0 ]; then
  # Add these SDKs for Windows or Linux
  sdks+=( orangebox blade episode1 bms )

  # Add more SDKs for Windows only
  if [ $iswin -eq 1 ]; then
    sdks+=( darkm swarm bgt eye contagion )
  fi
fi

# Check out a local copy as a proxy.
if [ ! -d "hl2sdk-proxy-repo" ]; then
  git clone --mirror https://github.com/alliedmodders/hl2sdk hl2sdk-proxy-repo
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

`python -c "import ambuild2"` || `python3 -c "import ambuild2"`
if [ $? -eq 1 ]; then
  repo="https://github.com/alliedmodders/ambuild"
  origin=
  branch=master
  name=ambuild
  checkout

  cd ambuild
  if [ $iswin -eq 1 ] || [ $ismac -eq 1 ]; then
    python setup.py install
  else
    python setup.py build
    echo "Installing AMBuild at the user level. Location can be: ~/.local/bin"
    python setup.py install --user
  fi
fi
