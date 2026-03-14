[CmdletBinding()]
param(
    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64",

    [ValidateSet("Auto", "VendorFallback", "PinnedArchives")]
    [string]$Source = "Auto",

    [switch]$Force
)

$ErrorActionPreference = "Stop"

$coreRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$depsRoot = Join-Path $coreRoot ".deps"
$includeRoot = Join-Path $depsRoot "include"
$libRoot = Join-Path $depsRoot "lib\$Platform"
$binRoot = Join-Path $depsRoot "bin\$Platform"
$downloadRoot = Join-Path $depsRoot "downloads"
$extractRoot = Join-Path $depsRoot "extract\$Platform"
$manifestPath = Join-Path $depsRoot "manifest.$Platform.json"
$vendorRoot = Join-Path $coreRoot "src\dep"
$vendorLibRoot = Join-Path $vendorRoot ("lib\" + $(if($Platform -eq "x64") { "X64" } else { "X32" }))

$pins = @{
    MariaDbVersion = "3.3.10"
    OpenSslVersion = "1.1.1w"
}

$downloads = @(
    @{
        Name = "mariadb"
        Url = "https://downloads.mariadb.com/Connectors/c/connector-c-$($pins.MariaDbVersion)/mariadb-connector-c-$($pins.MariaDbVersion)-winx64.zip"
        Archive = Join-Path $downloadRoot "mariadb-connector-c-$($pins.MariaDbVersion)-$Platform.zip"
    },
    @{
        Name = "openssl"
        Url = "https://www.openssl.org/source/openssl-$($pins.OpenSslVersion).tar.gz"
        Archive = Join-Path $downloadRoot "openssl-$($pins.OpenSslVersion).tar.gz"
    }
)

function Ensure-Directory {
    param([string]$Path)

    New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Reset-Directory {
    param([string]$Path)

    if(Test-Path $Path) {
        Remove-Item -Recurse -Force $Path
    }

    Ensure-Directory -Path $Path
}

function Copy-FileToPath {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($DestinationPath))
    Copy-Item -Force -Path $SourcePath -Destination $DestinationPath
}

function Copy-DirectoryContents {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    Ensure-Directory -Path $DestinationPath
    Copy-Item -Force -Recurse -Path (Join-Path $SourcePath "*") -Destination $DestinationPath
}

function Get-FirstExistingPath {
    param([string[]]$Candidates)

    foreach($candidate in $Candidates) {
        if([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        if(Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Find-FileRecursive {
    param(
        [string]$Root,
        [string[]]$Names
    )

    if(-not (Test-Path $Root)) {
        return $null
    }

    foreach($name in $Names) {
        $match = Get-ChildItem -Path $Root -Recurse -File -Filter $name | Select-Object -First 1
        if($match) {
            return $match.FullName
        }
    }

    return $null
}

function Expand-ArchiveToPath {
    param(
        [string]$ArchivePath,
        [string]$DestinationPath
    )

    Reset-Directory -Path $DestinationPath

    if($ArchivePath.EndsWith(".zip")) {
        Expand-Archive -Path $ArchivePath -DestinationPath $DestinationPath -Force
        return
    }

    if($ArchivePath.EndsWith(".tar.gz")) {
        & tar -xzf $ArchivePath -C $DestinationPath
        if($LASTEXITCODE -ne 0) {
            throw "Failed to extract archive '$ArchivePath' with tar."
        }
        return
    }

    throw "Unsupported archive format for '$ArchivePath'."
}

function Get-FileHashOrNull {
    param([string]$Path)

    if(-not (Test-Path $Path)) {
        return $null
    }

    return (Get-FileHash -Algorithm SHA256 -Path $Path).Hash
}

function Stage-VendorFallback {
    Write-Host "Staging repo-vendored dependency fallback for $Platform"

    Ensure-Directory -Path $includeRoot
    Ensure-Directory -Path $libRoot
    Ensure-Directory -Path $binRoot

    Copy-DirectoryContents -SourcePath (Join-Path $vendorRoot "mysql") -DestinationPath (Join-Path $includeRoot "mysql")
    Copy-DirectoryContents -SourcePath (Join-Path $vendorRoot "openssl") -DestinationPath (Join-Path $includeRoot "openssl")

    $mysqlLib = Get-FirstExistingPath -Candidates @(
        (Join-Path $vendorLibRoot "libmysql.lib"),
        (Join-Path $vendorLibRoot "libmySQL.lib")
    )
    if(-not $mysqlLib) {
        throw "Could not find a MySQL import library under '$vendorLibRoot'."
    }

    $cryptoLib = Get-FirstExistingPath -Candidates @(
        (Join-Path $vendorLibRoot "libeay32.lib"),
        (Join-Path $vendorLibRoot "libcrypto.lib")
    )
    if(-not $cryptoLib) {
        throw "Could not find an OpenSSL import library under '$vendorLibRoot'."
    }

    Copy-FileToPath -SourcePath $mysqlLib -DestinationPath (Join-Path $libRoot "libmysql.lib")
    Copy-FileToPath -SourcePath $cryptoLib -DestinationPath (Join-Path $libRoot "libeay32.lib")

    $runtimeCandidates = @()
    if($Platform -eq "x64") {
        $runtimeCandidates += @(
            (Join-Path $coreRoot "bin\Debug_x64\libmysql.dll"),
            (Join-Path $coreRoot "bin\Release_x64\libmysql.dll"),
            (Join-Path $coreRoot "bin\debug_x64\libmysql.dll"),
            (Join-Path $coreRoot "bin\release_x64\libmysql.dll")
        )
    } else {
        $runtimeCandidates += @(
            (Join-Path $coreRoot "bin\Debug\libmysql.dll"),
            (Join-Path $coreRoot "bin\Release\libmysql.dll"),
            (Join-Path $coreRoot "bin\debug\libmysql.dll"),
            (Join-Path $coreRoot "bin\release\libmysql.dll")
        )
    }

    $mysqlDll = Get-FirstExistingPath -Candidates $runtimeCandidates
    if($mysqlDll) {
        Copy-FileToPath -SourcePath $mysqlDll -DestinationPath (Join-Path $binRoot "libmysql.dll")
    } else {
        Write-Warning "No libmysql.dll runtime was found for $Platform. The import library was staged, but runtime deployment remains manual."
    }

    $cryptoCandidates = @()
    if($Platform -eq "x64") {
        $cryptoCandidates += @(
            (Join-Path $coreRoot "bin\Debug_x64\libeay32.dll"),
            (Join-Path $coreRoot "bin\Release_x64\libeay32.dll"),
            (Join-Path $coreRoot "bin\debug_x64\libeay32.dll"),
            (Join-Path $coreRoot "bin\release_x64\libeay32.dll")
        )
    } else {
        $cryptoCandidates += @(
            (Join-Path $coreRoot "bin\Debug\libeay32.dll"),
            (Join-Path $coreRoot "bin\Release\libeay32.dll"),
            (Join-Path $coreRoot "bin\debug\libeay32.dll"),
            (Join-Path $coreRoot "bin\release\libeay32.dll")
        )
    }

    $cryptoDll = Get-FirstExistingPath -Candidates $cryptoCandidates
    if($cryptoDll) {
        Copy-FileToPath -SourcePath $cryptoDll -DestinationPath (Join-Path $binRoot "libeay32.dll")
    } else {
        Write-Warning "No libeay32.dll runtime was found for $Platform. The import library was staged, but runtime deployment remains manual."
    }

    return @{
        Mode = "vendor-fallback"
        MariaDbArchive = $null
        OpenSslArchive = $null
    }
}

function Stage-PinnedArchives {
    Write-Host "Staging pinned archives for $Platform"

    foreach($download in $downloads) {
        if($Force -or -not (Test-Path $download.Archive)) {
            Write-Host "Downloading $($download.Name) from $($download.Url)"
            Invoke-WebRequest -Uri $download.Url -OutFile $download.Archive
        }
    }

    $mariaExtractRoot = Join-Path $extractRoot "mariadb"
    $opensslExtractRoot = Join-Path $extractRoot "openssl"

    Expand-ArchiveToPath -ArchivePath $downloads[0].Archive -DestinationPath $mariaExtractRoot
    Expand-ArchiveToPath -ArchivePath $downloads[1].Archive -DestinationPath $opensslExtractRoot

    $mariaHeader = Find-FileRecursive -Root $mariaExtractRoot -Names @("mysql.h")
    $mariaLibrary = Find-FileRecursive -Root $mariaExtractRoot -Names @("libmysql.lib", "libmariadb.lib", "mariadbclient.lib")
    $mariaRuntime = Find-FileRecursive -Root $mariaExtractRoot -Names @("libmysql.dll", "libmariadb.dll")

    if(-not $mariaHeader -or -not $mariaLibrary) {
        throw "Could not locate the MariaDB Connector/C headers and library in '$mariaExtractRoot'."
    }

    $mariaIncludeRoot = Split-Path $mariaHeader -Parent
    if((Split-Path $mariaIncludeRoot -Leaf) -eq "mysql") {
        Copy-DirectoryContents -SourcePath $mariaIncludeRoot -DestinationPath (Join-Path $includeRoot "mysql")
    } else {
        Copy-DirectoryContents -SourcePath $mariaIncludeRoot -DestinationPath (Join-Path $includeRoot "mysql")
    }

    Copy-FileToPath -SourcePath $mariaLibrary -DestinationPath (Join-Path $libRoot "libmysql.lib")
    if($mariaRuntime) {
        Copy-FileToPath -SourcePath $mariaRuntime -DestinationPath (Join-Path $binRoot "libmysql.dll")
    } else {
        Write-Warning "MariaDB archive did not contain a runtime DLL. Build-time staging completed, but runtime deployment remains manual."
    }

    $opensslHeader = Find-FileRecursive -Root $opensslExtractRoot -Names @("opensslv.h")
    $opensslLibrary = Find-FileRecursive -Root $opensslExtractRoot -Names @("libeay32.lib", "libcrypto.lib")
    $opensslRuntime = Find-FileRecursive -Root $opensslExtractRoot -Names @("libeay32.dll", "libcrypto*.dll")

    if(-not $opensslHeader -or -not $opensslLibrary) {
        throw "Could not locate the OpenSSL headers and import library in '$opensslExtractRoot'. A source tarball alone is not enough for Windows staging."
    }

    $opensslIncludeRoot = Split-Path $opensslHeader -Parent
    if((Split-Path $opensslIncludeRoot -Leaf) -eq "openssl") {
        Copy-DirectoryContents -SourcePath $opensslIncludeRoot -DestinationPath (Join-Path $includeRoot "openssl")
    } else {
        Copy-DirectoryContents -SourcePath $opensslIncludeRoot -DestinationPath (Join-Path $includeRoot "openssl")
    }

    Copy-FileToPath -SourcePath $opensslLibrary -DestinationPath (Join-Path $libRoot "libeay32.lib")
    if($opensslRuntime) {
        Copy-FileToPath -SourcePath $opensslRuntime -DestinationPath (Join-Path $binRoot "libeay32.dll")
    } else {
        Write-Warning "OpenSSL archive did not contain a runtime DLL. Build-time staging completed, but runtime deployment remains manual."
    }

    return @{
        Mode = "pinned-archives"
        MariaDbArchive = $downloads[0].Archive
        OpenSslArchive = $downloads[1].Archive
    }
}

function Write-Manifest {
    param([hashtable]$Result)

    $manifest = [ordered]@{
        platform = $Platform
        source = $Result.Mode
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
        pins = [ordered]@{
            mariadbConnectorC = $pins.MariaDbVersion
            openssl = $pins.OpenSslVersion
        }
        stagedArtifacts = [ordered]@{
            mysqlInclude = [ordered]@{
                path = (Join-Path $includeRoot "mysql")
            }
            opensslInclude = [ordered]@{
                path = (Join-Path $includeRoot "openssl")
            }
            mysqlImportLibrary = [ordered]@{
                path = (Join-Path $libRoot "libmysql.lib")
                sha256 = (Get-FileHashOrNull -Path (Join-Path $libRoot "libmysql.lib"))
            }
            opensslImportLibrary = [ordered]@{
                path = (Join-Path $libRoot "libeay32.lib")
                sha256 = (Get-FileHashOrNull -Path (Join-Path $libRoot "libeay32.lib"))
            }
            mysqlRuntime = [ordered]@{
                path = (Join-Path $binRoot "libmysql.dll")
                sha256 = (Get-FileHashOrNull -Path (Join-Path $binRoot "libmysql.dll"))
            }
            opensslRuntime = [ordered]@{
                path = (Join-Path $binRoot "libeay32.dll")
                sha256 = (Get-FileHashOrNull -Path (Join-Path $binRoot "libeay32.dll"))
            }
        }
        archives = [ordered]@{
            mariadb = $Result.MariaDbArchive
            openssl = $Result.OpenSslArchive
        }
    }

    $manifest | ConvertTo-Json -Depth 6 | Set-Content -Encoding ASCII -Path $manifestPath
}

Ensure-Directory -Path $depsRoot
Ensure-Directory -Path $downloadRoot
Ensure-Directory -Path $extractRoot

if($Force) {
    Reset-Directory -Path $includeRoot
    Reset-Directory -Path $libRoot
    Reset-Directory -Path $binRoot
} else {
    Ensure-Directory -Path $includeRoot
    Ensure-Directory -Path $libRoot
    Ensure-Directory -Path $binRoot
}

$resolvedSource = $Source
if($resolvedSource -eq "Auto") {
    if(($downloads | Where-Object { Test-Path $_.Archive }).Count -eq $downloads.Count) {
        $resolvedSource = "PinnedArchives"
    } else {
        $resolvedSource = "VendorFallback"
    }
}

$result = switch($resolvedSource) {
    "PinnedArchives" { Stage-PinnedArchives }
    "VendorFallback" { Stage-VendorFallback }
    default { throw "Unsupported bootstrap source '$resolvedSource'." }
}

Write-Manifest -Result $result

Write-Host "Dependency staging complete for $Platform"
Write-Host "  Source mode: $($result.Mode)"
Write-Host "  Include root: $includeRoot"
Write-Host "  Library root: $libRoot"
Write-Host "  Runtime root: $binRoot"
Write-Host "  Manifest: $manifestPath"
