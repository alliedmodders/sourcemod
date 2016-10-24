<#
.SYNOPSIS
    Downloads dependencies for compiling SourceMod.
.PARAMETER SDKs
    List of HL2SDK branch names to downloads.
#>

[CmdletBinding()]
param(
    [string[]]$SDKs = @(
        'csgo',
        'hl2dm',
        'nucleardawn',
        'l4d2',
        'dods',
        'l4d',
        'css',
        'tf2',
        'insurgency',
        'sdk2013',
        'dota'
        )
)

Function Checkout-Repo
{
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$Branch,
        [Parameter(Mandatory=$true)][string]$Repo,
        [string]$Origin
    )

    If (-not (Test-Path $Name -PathType Container))
    {
        & git clone $Repo -b $Branch $Name 2>&1 | Write-Host
        If ($Origin)
        {
            Set-Location $Name
            & git  remote rm origin 2>&1 | Write-Host
            & git remote add origin $Origin 2>&1 | Write-Host
            Set-Location ..
        }
    }
    Else
    {
        Set-Location $Name
        & git checkout $Branch 2>&1 | Write-Host
        & git pull origin $Branch 2>&1 | Write-Host
        Set-Location ..
    }
}

if (-not (Test-Path "sourcemod" -PathType Container))
{
    Write-Error "Could not find a SourceMod repository; make sure you aren't running this script inside it."
    Exit 1
}

Checkout-Repo -Name "mmsource-1.10" -Branch "1.10-dev" -Repo "https://github.com/alliedmodders/metamod-source.git"

if (-not (Test-Path "hl2sdk-proxy-repo" -PathType Container))
{
    & git clone --mirror https://github.com/alliedmodders/hl2sdk hl2sdk-proxy-repo 2>&1 | Write-Host
}
else
{
    Set-Location hl2sdk-proxy-repo
    & git fetch 2>&1 | Write-Host
    Set-Location ..
}

$SDKS | % {
    Checkout-Repo -Name "hl2sdk-$_" -Branch $_ "hl2sdk-proxy-repo" -Repo "https://github.com/alliedmodders/hl2sdk.git"
}

Checkout-Repo -Name "ambuild" -Branch "master" -Repo "https://github.com/alliedmodders/ambuild.git"
Set-Location ambuild
& python setup.py install

Set-Location ..