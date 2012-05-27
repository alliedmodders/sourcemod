#!/bin/bash

# Do not run this file directly. It is meant to be called by other scripts

SM_PATH=/home/gdc/sourcemod-central

ENGINE_BIN=${ENGINE_PATH}/bin/engine
GAME_BIN=${ENGINE_PATH}/${GAME_DIR}/bin/server
STEAMINF=${ENGINE_PATH}/${GAME_DIR}/steam.inf
BIN_EXT=""
if [ $MOD == 1 ] ; then
	BIN_EXT="_i486"
fi

echo -e "Checking game ${GAME_DIR}...\n"

if [ $MOD == 0 ] && [ "$1" == "auto" ] ; then
	./updatecheck.pl "${STEAMINF}"
	if [ $? -ne 0 ] ; then
		exit 1
	elif [ ! -e ${STEAMINF}.new ] ; then
		echo -e "Update (maybe) available but no steam.inf.new written!\n"
	fi
fi

export RDTSC_FREQUENCY="disabled"
export LD_LIBRARY_PATH=".:${ENGINE_PATH}/bin:$LD_LIBRARY_PATH"

UPDATE=0
if [ "$1" == "update" ] ; then
	UPDATE=1
elif [ "$1" == "auto" ] ; then
	UPDATE=1
fi

if [ ${UPDATE} -eq 1 ] ; then
	cd ${DD_PATH}

	if [ $MOD == 0 ] ; then
		#workaround for DD1 "bug" (won't redownload file of same name/size)
		rm -f ${STEAMINF}.old
		mv ${STEAMINF} ${STEAMINF}.old
	fi

	for i in 1 2 3 4 5
	do
		mono DepotDownloader.exe     \
			-game "${DD_GAME}"   \
			-dir ${DD_DIR}       \
			-filelist server.txt \
			-all-platforms

		if [ $? == 0 ] ; then
			break
		elif [ $i == 5 ] ; then
			echo -e "Update failed five times; giving up  ¯\(º_º)/¯\n"
			break
		fi

		echo -e "Update failed. Trying again in 30 seconds...\n"
		sleep 30
	done
fi

if [ "$1" == "auto" ] ; then
	DOWNLOADED_VER=`grep -E "^(Patch)?Version=(([0-9]\.?)+)" ${STEAMINF} | grep -Eo "([0-9]\.?)+" | sed s/[^0-9]//g`
	EXPECTED_VER=`cat ${STEAMINF}.new`

	if [ ${DOWNLOADED_VER} != ${EXPECTED_VER} ] ; then
		echo -e "Download resulted with version ${DOWNLOADED_VER}, but expected ${EXPECTED_VER}. Exiting.\n"
		exit 1
	fi
fi

# update sourcemod
cd ${SM_PATH}/tools/gdc-psyfork
hg pull -u

echo -e "\n"
for i in "${gamedata_files[@]}"
do
	./Release/gdc \
	        -g ${GAME_DIR} \
        	-e ${ENGINE_NAME} \
	        -f ${SM_PATH}/gamedata/$i \
        	-b ${GAME_BIN}${BIN_EXT}.so \
	        -x ${ENGINE_BIN}${BIN_EXT}.so \
	        -w ${GAME_BIN}.dll \
	        -y ${ENGINE_BIN}.dll
	echo -e "------------------------------------------------------\n"
	echo -e "\n"
done

if [ ! -e ${STEAMINF} ] ; then
	mv ${STEAMINF}.old ${STEAMINF}
fi

if [ "$1" == "auto" ] && [ -e ${STEAMINF}.new ] ; then
	rm ${STEAMINF}.new
fi

exit 0
