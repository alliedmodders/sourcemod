#!/bin/bash

# Path to DepotDownloader
DD_PATH=/home/gdc/dd

# Path for DepotDownloader to download to, relative to DD_PATH
DD_DIR=source

# Game to use for DepotDownloader -game param
DD_GAME="Counter-Strike Source"

# Absolute path to game's engine directory
ENGINE_PATH=${DD_PATH}/${DD_DIR}/css

# Game's directory name
GAME_DIR=cstrike

# SM gamedata engine name
ENGINE_NAME=orangebox_valve

# List of gamedata files to run checks on
gamedata_files=(
	"sdktools.games/game.cstrike.txt"
	"sdktools.games/engine.ep2valve.txt"
	"sm-cstrike.games/game.css.txt"
)

# Is game a 2006/2007 "mod" ?
# If so, bin names are adjusted with _i486 suffix and no update check will be done
MOD=0

# DO NOT EDIT BELOW THIS LINE

source ./gdc_core.sh $1 $2 $3 $4

exit $?
