param(
    [switch]$SUPPRESS_BUILD,
    [switch]$EXPORT_BINARIES,

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
& $LOCAL_PATH/test.ps1 -APP_ID_ROOT:500 -APP_ID:222840 -APP_SM_ENGINE:"l4d" -APP_ENGINE:"left4dead" -BASE_PATH:$BASE_PATH -SUPPRESS_BUILD:$SUPPRESS_BUILD -EXPORT_BINARIES:$EXPORT_BINARIES