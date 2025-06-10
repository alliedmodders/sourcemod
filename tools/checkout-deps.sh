#!/usr/bin/env bash
# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

trap "exit" INT

# List of HL2SDK branch names to download.
# ./checkout-deps.sh -s tf2,css
while getopts ":s:" opt; do
  case $opt in
    s) IFS=', ' read -r -a sdks <<< "$OPTARG"
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
branch=master
repo="https://github.com/alliedmodders/metamod-source"
origin=
checkout

if [ -z ${sdks+x} ]; then
  sdks=( csgo hl2dm nucleardawn l4d2 dods l4d css tf2 insurgency sdk2013 doi )

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
