#!/bin/bash

test -e compiled || mkdir compiled

if [[ $# -ne 0 ]]
then
    for i in "$@"; 
    do
        smxfile="`echo $i | sed -e 's/\.sp$/\.smx/'`";
	    echo -n "Compiling $i...";
	    ./spcomp $i -ocompiled/$smxfile
    done
else

for sourcefile in *.sp
do
	smxfile="`echo $sourcefile | sed -e 's/\.sp$/\.smx/'`"
	echo -n "Compiling $sourcefile ..."
	./spcomp $sourcefile -ocompiled/$smxfile
done
fi
