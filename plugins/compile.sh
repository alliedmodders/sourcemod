#!/bin/bash

test -e compiled || mkdir compiled

for sourcefile in *.sp
do
	smxfile="`echo $sourcefile | sed -e 's/\.sp$/\.smx/'`"
	echo -n "Compiling $sourcefile ..."
	./spcomp $sourcefile -ocompiled/$smxfile
done

