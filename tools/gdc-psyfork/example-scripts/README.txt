Some example scripts for using this fork of gdc. These are based on some of my live scripts.

Hopefully these are helpful to someone.

gdc_auto.sh
A wrapper script, meant to be cronned. It will run the game's gdc script for games listed in the "games" array. If an update is was detected, it will write the gdc results and a log file as well as email everyone in the "email_all" and "email_<game>" arrays.

gdc_core.sh
This is meant to be called by gdc game scripts, rather than invoked directly. It handles logic common to all games, including invokation of the update check, update download, and gdc.

updatecheck.pl </path/to/steam.inf>
This is used by the gdc_core script, but can also be used standalone. Given the path to a steam.inf file, it will check to see if the game is up to date. Exit codes are as follows (zero and one would ideally be reversed, but I don't want to break existing scripts): 0 - Updated needed. 1 - Up to date. 2 - Failed to parse steam.inf. 3 - Steam Web API gave bad response code (maybe down?)

gdc_<game>.sh, gdc_cstrike.sh [update|auto]
The included example gdc_cstrike.sh is a game script. It can be copied to create others. The game script defines vars specific to the game, such as download name, gamedir name, path to game relative to download dir, etc.. By default, it will not attempt any update; it will just check using the existing files. Ran with "auto", it will update if necessary, and stop if up to date. Ran with "update" will force an update before running.