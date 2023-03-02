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


# Test Day of Defeat: Source
& $LOCAL_PATH/test.ps1 -APP_ID_ROOT:673560 -APP_ID:673990 -APP_ENGINE:"iosoccer" -APP_SM_ENGINE:"ios" -BINARY_APPEND:"_i486" -BASE_PATH:$BASE_PATH -SUPPRESS_BUILD:$SUPPRESS_BUILD -EXPORT_BINARIES:$EXPORT_BINARIES