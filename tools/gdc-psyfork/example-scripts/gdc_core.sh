#!/bin/bash

SCRIPT_PATH=/users/psychonic/gdc

# with trailing slash or undefined for system default
MONO_BIN_PATH=/apps/mono-2.10.9/bin/

DD_PATH=${SCRIPT_PATH}/dd
ENGINE_PATH=${DD_PATH}/${ENGINE_PATH_FROM_DD}

SM_PATH=${SCRIPT_PATH}/sourcemod-central
SMRCON_PATH=${SCRIPT_PATH}/SMRCon

# Do not run this file directly. It is meant to be called by other scripts

ENGINE_BIN=${ENGINE_PATH}/bin/engine
GAME_BIN=${ENGINE_PATH}/${GAME_DIR}/bin/server
STEAMINF=${ENGINE_PATH}/${GAME_DIR}/steam.inf

GDC_PATH=${SM_PATH}/tools/gdc-psyfork
GDC_BIN=${GDC_PATH}/Release/gdc

if [ "${GAMEDATA_DIR}" == "" ] ; then
	GAMEDATA_DIR=${GAME_DIR}
fi

if [ $MOD == 1 ] ; then
	BIN_EXT="_i486"
fi

echo -e "Checking game ${GAME_DIR}...\n"

if [ $MOD == 0 ] && [ "$1" == "auto" ] ; then
	UPDATE_RES=`./updatecheck.pl "${STEAMINF}"`
	if [ $? -ne 0 ] ; then
		exit 1
	fi
	EXPECTED_VER=`echo ${UPDATE_RES} | egrep -o '([0-9]+)$'`
	echo Expecting version ${EXPECTED_VER}
fi

export RDTSC_FREQUENCY="disabled"
export LD_LIBRARY_PATH="${ENGINE_PATH}:${ENGINE_PATH}/bin:$LD_LIBRARY_PATH"

UPDATE=0
if [ "$1" == "update" ] ; then
	UPDATE=1
elif [ "$1" == "auto" ] ; then
	UPDATE=1
fi

if [ ${UPDATE} -eq 1 ] ; then
	cd ${DD_PATH}

	if [ "${DD_NEEDS_AUTH}" != "" ] ; then
		if [ "${DD_GAME}" != "" ] ; then
			DD_OPT_AUTH=`tr '\r\n' ' ' < dd-login-info.txt`
		elif [ "${DD_APP}" != "" ] ; then
			DD_OPT_AUTH=`tr '\r\n' ' ' < dd-login-info.txt`
		else
			echo "Error: neither DD_GAME nor DD_APP are set!"
			exit 1
		fi
	fi

	if [ "${DD_BETA}" != "" ] ; then
		DD_OPT_BETA="-beta ${DD_BETA}"
		if [ "${DD_BETA_PASSWORD}" != "" ] ; then
			DD_OPT_BETA_PASSWORD="-betapassword ${DD_BETA_PASSWORD}"
		fi
	fi

	for i in 1 2 3 4 5
	do
		if [ "${DD_GAME}" != "" ] ; then
			${MONO_BIN_PATH}mono DepotDownloader.exe     \
				-game "${DD_GAME}"   \
				-dir ${DD_DIR}       \
				-filelist server.txt \
				-all-platforms       \
				-no-exclude          \
				${DD_OPT_CELL}       \
				${DD_OPT_AUTH}
		else
			${MONO_BIN_PATH}mono DepotDownloader.exe     \
				-app "${DD_APP}"     \
				-dir ${DD_DIR}       \
				-filelist server.txt \
				-all-platforms       \
				-no-exclude          \
				${DD_OPT_CELL}       \
				${DD_OPT_AUTH}       \
				${DD_OPT_BETA}       \
				${DD_OPT_BETA_PASSWORD}
		fi

		echo

		if [ $? == 0 ] ; then
			break
		elif [ $i == 5 ] ; then
			echo Update failed five times; welp
			break
		fi

		echo -e "Update failed. Trying again in 30 seconds...\n"
		sleep 30
	done
fi

if [ "$1" == "auto" ] ; then
	DOWNLOADED_VER=`grep -E "^(Patch)?Version=(([0-9]\.?)+)" ${STEAMINF} | grep -Eo "([0-9]\.?)+" | sed s/[^0-9]//g`

	if [ ${DOWNLOADED_VER} != ${EXPECTED_VER} ] ; then
		echo Download resulted with version ${DOWNLOADED_VER}, but expected ${EXPECTED_VER}. Exiting.
		exit 1
	fi
fi

# update game-specific
cd ${SCRIPT_PATH}
GAME_SCRIPT_NAME=`echo $0 | sed s/\.sh$//`
echo checking to see if  ${GAME_SCRIPT_NAME}_repos.sh  exists
if [ -e ${GAME_SCRIPT_NAME}_repos.sh ] ; then
	./${GAME_SCRIPT_NAME}_repos.sh
fi

# update sourcemod
echo Updating SourceMod repo
cd ${SM_PATH}
hg pull -u

echo -e "\n"

cd ${ENGINE_PATH}

for i in "${gamedata_files[@]}"
do
	NO_SYMTABLE=

	readelf --sections ${GAME_BIN}${BIN_EXT}.so | grep --quiet .symtab
	if [ "${PIPESTATUS[1]}" != "0" ] ; then
		NO_SYMTABLE=" -n"
	fi

	readelf --sections ${GAME_BIN}${BIN_EXT}.so | grep --quiet .strtab
	if [ "${PIPESTATUS[1]}" != "0" ] ; then
			NO_SYMTABLE=" -n"
	fi

	# having an issue upon exit after loading some source2007 server bins, invalid free in sendtable dtor, idk. suppress.
	MALLOC_CHECK_=0 ${GDC_BIN} \
		-g ${GAMEDATA_DIR} \
		-e ${ENGINE_NAME} \
		-f ${SM_PATH}/gamedata/$i \
		-b ${GAME_BIN}${BIN_EXT}.so \
		-x ${ENGINE_BIN}${BIN_EXT}.so \
		-w ${GAME_BIN}.dll \
		-y ${ENGINE_BIN}.dll \
		-s ${GDC_PATH}/symbols.txt \
		${NO_SYMTABLE}
	echo -e "------------------------------------------------------\n"
done

exit 0
