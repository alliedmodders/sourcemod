param(
    [switch]$SUPPRESS_BUILD,

    #   A path to the root "sourcemod" directory.
    $BASE_PATH
    ) 
Write-Host "SUPPRESS_BUILD: $SUPPRESS_BUILD"
Write-Host "BASE_PATH: $BASE_PATH"

if ($BASE_PATH -eq $Null)
{
    Write-Host "* Using working directory as base path"
    $BASE_PATH = Get-Location
}

$LOCAL_PATH = "$BASE_PATH/scripts/test"


# Test CS:GO
& $LOCAL_PATH/test.ps1 -APP_ID_ROOT:730 -APP_ID:740 -APP_ENGINE:"csgo" -BASE_PATH:$BASE_PATH -SUPPRESS_BUILD:$SUPPRESS_BUILD