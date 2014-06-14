#!/bin/bash
cd "$(dirname "$0")"

test -e compiled || mkdir compiled

if [[ $# -ne 0 ]]; then
	for i in "$@"; 
	do
		smxfile="`echo $i | sed -e 's/\.sp$/\.smx/'`";
		echo -e "Compiling $i...";
		./spcomp $i -ocompiled/$smxfile
		RETVAL=$?
		if [ $RETVAL -ne 0 ]; then
			exit 1;
		fi
	done
else
	for sourcefile in *.sp
	do
		smxfile="`echo $sourcefile | sed -e 's/\.sp$/\.smx/'`"
		echo -e "Compiling $sourcefile ..."
		./spcomp $sourcefile -ocompiled/$smxfile
		RETVAL=$?
		if [ $RETVAL -ne 0 ]; then
			exit 1;
		fi
	done
fi
