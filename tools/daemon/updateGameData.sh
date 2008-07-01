#!/bin/bash

FILENAME=""
SUMSFILE=""

if [ -n "$1" ]
then
	FILENAME="$1.txt"
	SUMSFILE="$1.sums"
else
	echo "Need to specify a gamedata filename"
	exit -1
fi


if [ -s $FILENAME ]
then
	#run ./gamedatamd5 on this file and pipe output+filename into $1.sums
	MD5=`./gamedatamd5 $FILENAME`
	#need to stop here if gamedatamd5 failed. (returns -1 and prints to stderr)
	echo "$MD5" > "$SUMSFILE"
	echo "$FILENAME" >> "$SUMSFILE"
	ln -s "$SUMSFILE" "$MD5"

	exit 0
fi

echo "File $FILENAME not found!"

exit -1