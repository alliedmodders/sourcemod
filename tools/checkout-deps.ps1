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
        ),
    [switch]$NoMariaDB
)

$MARIADB_CONNECTOR_C_VERSION = if ($env:MARIADB_CONNECTOR_C_VERSION) { $env:MARIADB_CONNECTOR_C_VERSION } else { '3.4.9' }
$MARIADB_CONNECTOR_C_RELEASE = if ($env:MARIADB_CONNECTOR_C_RELEASE) { $env:MARIADB_CONNECTOR_C_RELEASE } else { '3.4.9-sm.5' }
$MARIADB_CONNECTOR_C_REPOSITORY = 'alliedmodders/mariadb-connector-c'

Function Get-MariaDBConnectorC
{
    param(
        [Parameter(Mandatory=$true)][string]$Architecture
    )

    $folder = "mariadb-connector-c-$MARIADB_CONNECTOR_C_VERSION-$Architecture"
    if (Test-Path $folder -PathType Container)
    {
        return
    }

    $releaseUrl = "https://github.com/$MARIADB_CONNECTOR_C_REPOSITORY/releases/download/v$MARIADB_CONNECTOR_C_RELEASE"
    $archiveName = "mariadb-connector-c-$MARIADB_CONNECTOR_C_VERSION-windows-$Architecture.zip"
    $checksumPath = Join-Path (Resolve-Path '.') 'SHA256SUMS'
    $archivePath = Join-Path (Resolve-Path '.') $archiveName

    Invoke-WebRequest -Uri "$releaseUrl/SHA256SUMS" -OutFile $checksumPath
    Invoke-WebRequest -Uri "$releaseUrl/$archiveName" -OutFile $archivePath

    $expected = Get-Content -LiteralPath $checksumPath |
        ForEach-Object {
            $fields = $_ -split '\s+'
            if ($fields.Length -ge 2 -and $fields[-1].TrimStart('*') -eq $archiveName)
            {
                $fields[0]
            }
        } |
        Select-Object -First 1
    if (-not $expected)
    {
        throw "No SHA256 checksum was published for $archiveName."
    }

    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash
    if ($actual -ne $expected)
    {
        throw "SHA256 verification failed for $archiveName."
    }

    Expand-Archive -LiteralPath $archivePath -DestinationPath .
    if (-not (Test-Path $folder -PathType Container))
    {
        throw "MariaDB Connector/C archive did not contain $folder."
    }

    Remove-Item -LiteralPath $archivePath, $checksumPath
}

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

if (-not $NoMariaDB)
{
    Get-MariaDBConnectorC -Architecture 'x86'
    Get-MariaDBConnectorC -Architecture 'x86_64'
}

Get-Repository -Name "mmsource-1.12" -Branch "1.12-dev" -Repo "https://github.com/alliedmodders/metamod-source.git"

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
