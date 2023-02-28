param(
    [switch]$SUPPRESS_BUILD,
    [switch]$EXPORT_BINARIES,
    #   The "root app"
    #   Used for fetching the version number for the dedicated server 
    #   (which is used for build caching)
    $APP_ID_ROOT,
    #   The app ID to install
    $APP_ID,
    #   The app's engine name
    #   Eg, "csgo", "left4dead"
    $APP_ENGINE,
    #   Used when the engine name diverges from SM's code
    $APP_SM_ENGINE,
    #   A path to the root "sourcemod" directory.
    $BASE_PATH,
    #   A string to append to binaries
    #   Since some end in _srv, etc.
    $BINARY_APPEND
    ) 
Write-Host "SUPPRESS_BUILD: $SUPPRESS_BUILD"
Write-Host "APP_ID_ROOT: $APP_ID_ROOT"
Write-Host "APP_ID: $APP_ID"
Write-Host "APP_ENGINE: $APP_ENGINE"
Write-Host "APP_SM_ENGINE: $APP_SM_ENGINE"
Write-Host "BASE_PATH: $BASE_PATH"
Write-Host "BINARY_APPEND: $BINARY_APPEND"

if ($BASE_PATH -eq $Null)
{
    Write-Host "* Using working directory as base path"
    $BASE_PATH = Get-Location
}

if ($APP_SM_ENGINE -eq $Null)
{
    Write-Host "* Using app engine as SM engine slug"
    $APP_SM_ENGINE = $APP_ENGINE
}


$LOCAL_PATH = "$BASE_PATH/scripts/test"


Write-Host "* Fetching latest depot version..."
$uptodate = ConvertFrom-JSON( Invoke-WebRequest -Uri "http://api.steampowered.com/ISteamApps/UpToDateCheck/v1?appid=${APP_ID_ROOT}&version=0" )

$depot_version = $uptodate.response.required_version
Write-Host "* Latest depot version is $depot_version."

Write-Host "========================="
Write-Host "BUILDING DOCKER CONTAINER"
Write-Host "========================="

$docker_args = ""

if ($SUPPRESS_BUILD -eq $True)
{
    $docker_args = "$docker_args --quiet"
}

Write-Host "* Using Vanilla Docker"
iex "docker build $docker_args --build-arg DEPOT_VERSION=$depot_version --build-arg APP_ID=$APP_ID --build-arg DEPOT_ID=$APP_ID --file `"$LOCAL_PATH/Dockerfile`" -t sourcemod-gdc:latest `"$BASE_PATH`""

Write-Host "* Done!"

Write-Host "========================"
Write-Host "RUNNING GAMEDATA CHECKER"
Write-Host "========================"

if (Test-Path -PathType Container $LOCAL_PATH/logs)
{
    Write-Host "* Clearing logs"
    Remove-Item -Path $LOCAL_PATH/logs -Recurse
}

Write-Host "* Creating logs directory"
New-Item -Path $LOCAL_PATH/logs -Type Directory
New-Item -Path $LOCAL_PATH/logs/sm-cstrike.games -Type Directory
New-Item -Path $LOCAL_PATH/logs/core.games -Type Directory
New-Item -Path $LOCAL_PATH/logs/sdkhooks.games -Type Directory
New-Item -Path $LOCAL_PATH/logs/sdktools.games -Type Directory

Write-Host "* Modifying Lua Path"
$old_lua_path = $env:LUA_PATH
Write-Host "* LUA_PATH='$old_lua_path'"
[Environment]::SetEnvironmentVariable("LUA_PATH", ";;$LOCAL_PATH/?.lua", [System.EnvironmentVariableTarget]::Process)
Write-Host "* Using Vanilla Docker"



$full_gamedatafiles = Get-ChildItem -Path $BASE_PATH/gamedata -File -Name

# Find which gamedata files we need from the various folders

# Core: Only matching engine.$APP_SM_ENGINE
$coregames_gamedatafiles = Get-ChildItem -Path $BASE_PATH/gamedata/core.games -File -Name | Where-Object {($_ -eq "engine.$APP_SM_ENGINE.txt")} | ForEach-Object {"core.games/$_"}
# SDK things: match game.$APP_SM_ENGINE and engine.$APP_SM_ENGINE
$sdkhooks_gamedatafiles = Get-ChildItem -Path $BASE_PATH/gamedata/sdkhooks.games -File -Name | Where-Object {($_ -eq "engine.$APP_SM_ENGINE.txt") -or ($_ -eq "game.$APP_SM_ENGINE.txt")} | ForEach-Object {"sdkhooks.games/$_"}
$sdktools_gamedatafiles = Get-ChildItem -Path $BASE_PATH/gamedata/sdktools.games -File -Name | Where-Object {($_ -eq "engine.$APP_SM_ENGINE.txt") -or ($_ -eq "game.$APP_SM_ENGINE.txt")} | ForEach-Object {"sdktools.games/$_"}
# CStrike: Only game.$APP_SM_ENGINE
$smcstrike_gamedatafiles = Get-ChildItem -Path $BASE_PATH/gamedata/sm-cstrike.games -File -Name | Where-Object {($_ -eq "game.$APP_SM_ENGINE.txt") } | ForEach-Object {"sm-cstrike.games/$_"}

# Gamedatas that are shared across games.
$common_gamedatas = "core.games/common.games.txt", "sdkhooks.games/common.games.txt", "sdktools.games/common.games.txt" 

# Aggregate into a list
$gamedatafiles = $full_gamedatafiles + $coregames_gamedatafiles + $sdkhooks_gamedatafiles + $sdktools_gamedatafiles + $smcstrike_gamedatafiles + $common_gamedatas

$result_sum = 0

foreach ($gamedata in $gamedatafiles)
{
    docker run --rm sourcemod-gdc:latest -f /test/gamedata/$gamedata -g $APP_ENGINE -e $APP_SM_ENGINE -b /test/server/$APP_ENGINE/bin/server${BINARY_APPEND}.so -w /test/server/$APP_ENGINE/bin/server.dll -x /test/server/bin/engine${BINARY_APPEND}.so -y /test/server/bin/engine.dll -s /test/tools/gdc/symbols.txt > output.temp

    $result_process = Start-Process lua -ArgumentList "${LOCAL_PATH}/parse_results.lua $gamedata" -NoNewWindow -PassThru -Wait
    Copy-Item output.temp $LOCAL_PATH/logs/$gamedata.log
    $result_sum = $result_sum + $result_process.ExitCode
}

if ($EXPORT_BINARIES -eq $True)
{
    Write-Host "========================"
    Write-Host "EXPORTING DEPOT BINARIES"
    Write-Host "========================"

    $export_binaries = "bin/engine${BINARY_APPEND}.so", "bin/engine.dll", "$APP_ENGINE/bin/server${BINARY_APPEND}.so", "$APP_ENGINE/bin/server.dll"

    Write-Host "* Starting Export Container"
    docker run --name sourceforks-exporter-dummy sourcemod-gdc:latest > exporter.temp

    foreach ($binary in $export_binaries)
    {
        Write-Host "* Exporting $binary"
        docker cp sourceforks-exporter-dummy:test/server/$binary $LOCAL_PATH/bin
    }

    Write-Host "* Deleting Export Container"
    docker rm sourceforks-exporter-dummy
}

Write-Host "* Exiting with exit code $result_sum"
if ($result_sum -eq 0)
{
    Write-Host "* Exiting with successful exit code 0."
    Exit 0
}

Throw "An issue occured while checking the gamedata for $APP_ENGINE. Please check the GDC output to find the invalid signature."
