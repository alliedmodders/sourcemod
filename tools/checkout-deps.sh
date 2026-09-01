#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

trap "exit" INT

download_mariadb=1

mariadb_version=${MARIADB_CONNECTOR_C_VERSION:-3.4.9}
mariadb_release=${MARIADB_CONNECTOR_C_RELEASE:-3.4.9-sm.5}
mariadb_repo=alliedmodders/mariadb-connector-c

# List of HL2SDK branch names to download.
# ./checkout-deps.sh -s tf2,css
# Disable downloading of MariaDB Connector/C libraries.
# ./checkout-deps.sh -m
while getopts ":s:m" opt; do
  case $opt in
    s) IFS=', ' read -r -a sdks <<< "$OPTARG"
    ;;
    m) download_mariadb=0
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
    ;;
  esac
done

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

download_file ()
{
  if command -v wget >/dev/null; then
    wget -q "$1" -O "$2"
  elif command -v curl >/dev/null; then
    curl -fsSL "$1" -o "$2"
  else
    echo "Failed to locate wget or curl. Install one of these programs to download MariaDB Connector/C."
    exit 1
  fi
}

verify_checksum ()
{
  local archive=$1
  local sums=$2
  local entry
  entry=$(grep -E "[[:space:]][*]?$archive$" "$sums")
  if [ -z "$entry" ]; then
    echo "No checksum found for $archive"
    exit 1
  fi
  if command -v sha256sum >/dev/null; then
    printf '%s\n' "$entry" | sha256sum -c -
  else
    printf '%s\n' "$entry" | shasum -a 256 -c -
  fi
}

getmariadb ()
{
  local arch=$1
  local platform=$2
  local folder="mariadb-connector-c-$mariadb_version-$arch"
  local archive="mariadb-connector-c-$mariadb_version-$platform-$arch.$archive_ext"
  local release_url="https://github.com/$mariadb_repo/releases/download/v$mariadb_release"
  local sums=SHA256SUMS

  if [ ! -d "$folder" ]; then
    download_file "$release_url/$sums" "$sums"
    download_file "$release_url/$archive" "$archive"
    verify_checksum "$archive" "$sums"
    $decomp "$archive"
    if [ ! -d "$folder" ]; then
      echo "MariaDB Connector/C archive did not contain $folder"
      exit 1
    fi
    rm "$archive" "$sums"
  fi
}

if [ $download_mariadb -eq 1 ]; then
  mariadb_platform=linux
if [ $ismac -eq 1 ]; then
  mariadb_platform=macos
elif [ $iswin -eq 1 ]; then
  mariadb_platform=windows
fi
  getmariadb x86 "$mariadb_platform"
  getmariadb x86_64 "$mariadb_platform"
fi

checkout ()
{
  if [ ! -d "$name" ]; then
    git clone --recursive $repo -b $branch $name
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

name=mmsource-1.12
branch=1.12-dev
repo="https://github.com/alliedmodders/metamod-source"
origin=
checkout

if [ -z ${sdks+x} ]; then
  sdks=( csgo hl2dm nucleardawn l4d2 dods l4d css tf2 insurgency sdk2013 doi mcv )

  if [ $ismac -eq 0 ]; then
    # Add these SDKs for Windows or Linux
    sdks+=( orangebox blade episode1 bms pvkii )

    # Add more SDKs for Windows only
    if [ $iswin -eq 1 ]; then
      sdks+=( darkm swarm bgt eye contagion )
    fi
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

want_mock_sdk=0
for sdk in "${sdks[@]}"
do
  if [ "$sdk" == "mock" ]; then
    want_mock_sdk=1
    continue
  fi
  repo=hl2sdk-proxy-repo
  origin="https://github.com/alliedmodders/hl2sdk"
  name=hl2sdk-$sdk
  branch=$sdk
  checkout
done

if [ $want_mock_sdk -eq 1 ]; then
  name=hl2sdk-mock
  branch=master
  repo="https://github.com/alliedmodders/hl2sdk-mock"
  origin=
  checkout
fi

python_cmd=`command -v python3`
if [ -z "$python_cmd" ]; then
  python_cmd=`command -v python`

  if [ -z "$python_cmd" ]; then
    echo "No suitable installation of Python detected"
    exit 1
  fi
fi

$python_cmd -c "import ambuild2" 2>&1 1>/dev/null
if [ $? -eq 1 ]; then
  echo "AMBuild is required to build SourceMod"

  $python_cmd -m pip --version 2>&1 1>/dev/null
  if [ $? -eq 1 ]; then
    echo "The detected Python installation does not have PIP"
    echo "Installing the latest version of PIP available (VIA \"get-pip.py\")"

    get_pip="./get-pip.py"
    get_pip_url="https://bootstrap.pypa.io/get-pip.py"

    if [ `command -v wget` ]; then
      wget $get_pip_url -O $get_pip
    elif [ `command -v curl` ]; then
      curl -o $get_pip $get_pip_url
    else
      echo "Failed to locate wget or curl. Install one of these programs to download 'get-pip.py'."
      exit 1
    fi

    $python_cmd $get_pip
    if [ $? -eq 1 ]; then
      echo "Installation of PIP has failed"
      exit 1
    fi
  fi

  repo="https://github.com/alliedmodders/ambuild"
  origin=
  branch=master
  name=ambuild
  checkout

  if [ $iswin -eq 1 ]; then
    # Without first doing this explicitly, ambuild install fails on newer Python versions on Windows
    $python_cmd -m pip install wheel
    $python_cmd -m pip install ./ambuild
  elif [ $ismac -eq 1 ]; then
    $python_cmd -m pip install ./ambuild
  else
    echo "Installing AMBuild at the user level. Location can be: ~/.local/bin"
    $python_cmd -m pip install --user ./ambuild
  fi
fi
