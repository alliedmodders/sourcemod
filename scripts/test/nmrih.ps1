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


# Test No more room in hell
& $LOCAL_PATH/test.ps1 -APP_ID_ROOT:224260 -APP_ID:317670 -APP_ENGINE:"nmrih" -BINARY_APPEND:"_srv" -BASE_PATH:$BASE_PATH -SUPPRESS_BUILD:$SUPPRESS_BUILD -EXPORT_BINARIES:$EXPORT_BINARIES