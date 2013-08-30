# This should be run inside a folder that contains sourcemod, otherwise, it will checkout things into "sm-dependencies".

if [ ! -d "sourcemod-central" ]; then
  if [ ! -d "sourcemod-1.5" ]; then
    mkdir sm-dependencies
    cd sm-dependencies
  fi
fi

if [ ! -d "mysql-5.0" ]; then
  wget http://dev.mysql.com/get/Downloads/MySQL-5.6/mysql-5.6.13-linux-glibc2.5-i686.tar.gz/from/http://cdn.mysql.com/ -O mysql.tar.gz
  tar zxvf mysql.tar.gz
  mv mysql-5.6.13-linux-glibc2.5-i686 mysql-5.0
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

sdks=( csgo hl2dm nd l4d2 ob dods l4d css tf2 )

for sdk in "${sdks[@]}"
do
  name=hl2sdk-$sdk
  path=hl2sdks
  checkout
done

name=hl2sdk
path=hl2sdks
checkout

