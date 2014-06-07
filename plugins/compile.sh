#!/bin/bash
cd "$(dirname "$0")"

test -e compiled || mkdir compiled

if [[ $# -ne 0 ]]
then
	for sourcefile in "$@"; 
	do
		smxfile="`echo $sourcefile | sed -e 's/\.sp$/\.smx/'`";
		echo -e "\nCompiling $sourcefile...";
		./spcomp $i -ocompiled/$smxfile
    done
else

for sourcefile in *.sp
do
	smxfile="`echo $sourcefile | sed -e 's/\.sp$/\.smx/'`"
	echo -e "\nCompiling $sourcefile ..."
	./spcomp $sourcefile -ocompiled/$smxfile
done
fi

echo -e "\n"
read -n1 -rsp "Press any key to continue..."