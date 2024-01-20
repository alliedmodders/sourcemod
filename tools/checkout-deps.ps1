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
        'dota',
        'orangebox',
        'blade',
        'episode1',
        'bms',
        'darkm',
        'swarm',
        'bgt',
        'eye',
        'contagion',
        'doi',
        'pvkii'
        )
)

Function Get-Repository
{
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$Branch,
        [Parameter(Mandatory=$true)][string]$Repo,
        [string]$Origin
    )

    If (-not (Test-Path $Name -PathType Container))
    {
        & git clone --recursive $Repo -b $Branch $Name 2>&1 | Write-Host
        If ($Origin)
        {
            Set-Location $Name
            & git remote set-url origin $Origin 2>&1 | Write-Host
            Set-Location ..
        }
    }
    Else
    {
        Set-Location $Name
        If ($Origin)
        {
            & git remote set-url origin ..\$Repo 2>&1 | Write-Host
        }
        & git checkout $Branch 2>&1 | Write-Host
        & git pull origin $Branch 2>&1 | Write-Host
        If ($Origin)
        {
            & git remote set-url origin $Origin 2>&1 | Write-Host
        }
        Set-Location ..
    }
}

if (-not (Test-Path "sourcemod" -PathType Container))
{
    Write-Error "Could not find a SourceMod repository; make sure you aren't running this script inside it."
    Exit 1
}

Get-Repository -Name "mmsource-1.12" -Branch "master" -Repo "https://github.com/alliedmodders/metamod-source.git"

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

$SDKS | ForEach-Object {
    Get-Repository -Name "hl2sdk-$_" -Branch $_ -Repo "hl2sdk-proxy-repo" "https://github.com/alliedmodders/hl2sdk.git"
}

Get-Repository -Name "hl2sdk-mock" -Branch "master" -Repo "https://github.com/alliedmodders/hl2sdk-mock.git"

# Find a suitable installation of Python
$PYTHON_CMD = Get-Command 'python3' -ErrorAction SilentlyContinue
if ($NULL -eq $PYTHON_CMD)
{
    $PYTHON_CMD = Get-Command 'python' -ErrorAction SilentlyContinue
    if ($NULL -eq $PYTHON_CMD)
    {
        $PYTHON_CMD = Get-Command 'py' -ErrorAction SilentlyContinue
        if ($NULL -eq $PYTHON_CMD)
        {
            Write-Error 'No suitable installation of Python detected'
            Exit 1
        }
    }
}

$PYTHON_CMD = $PYTHON_CMD.Source # Convert the result into a string path.

& $PYTHON_CMD -c 'import ambuild2' 2>&1 1>$NULL
if ($LastExitCode -eq 1)
{
    Write-Host -ForegroundColor Red "AMBuild is required to build SourceMod"

    # Ensure PIP is installed, otherwise, install it.
    & $PYTHON_CMD -m pip --version 2>&1 1>$NULL # We use PIP's '--version' as it's the least verbose.
    if ($LastExitCode -eq 1) {
        Write-Host -ForegroundColor Red 'The detected Python installation does not have PIP'
        Write-Host 'Installing the latest version of PIP available (VIA "get-pip.py")'

        $GET_PIP = Join-Path $(Resolve-Path './') 'get-pip.py'
        Invoke-WebRequest -Uri "https://bootstrap.pypa.io/get-pip.py" -OutFile $GET_PIP

        & $PYTHON_CMD $GET_PIP
        if ($LastExitCode -eq 1) {
            Write-Error 'Installation of PIP has failed'
            Exit 1
        }
    }

    Get-Repository -Name "ambuild" -Branch "master" -Repo "https://github.com/alliedmodders/ambuild.git"
    & $PYTHON_CMD -m pip install ./ambuild
}
