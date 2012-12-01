#!/bin/bash

# Path to DepotDownloader
DD_PATH=/home/gdc/dd

# Path for DepotDownloader to download to, relative to DD_PATH
DD_DIR=csgo

# AppId to use for DepotDownloader for Steam3 content (steamcmd app_update number)
DD_APP=740

# Absolute path to game's engine directory
ENGINE_PATH=${DD_PATH}/${DD_DIR}/

# Game's directory name
GAME_DIR=csgo

# SM gamedata engine name
ENGINE_NAME=csgo

# List of gamedata files to run checks on
gamedata_files=(
	"sdktools.games/engine.csgo.txt"
	"sm-cstrike.games/game.csgo.txt"
)

# Is game a 2006/2007 "mod" ?
# If so, bin names are adjusted with _i486 suffix and no update check will be done
MOD=0

# DO NOT EDIT BELOW THIS LINE

source ./gdc_core.sh $1 $2 $3 $4

exit $?
