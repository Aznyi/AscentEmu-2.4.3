param(
    [Parameter(Mandatory = $true)]
    [string]$ClientPath,

    [string]$OutputPath = ".\\vmaps"
)

$extractor = Join-Path $PSScriptRoot "bin\\Win32\\ReleaseAS\\vmapextract_v2.exe"
$assembler = Join-Path (Split-Path $PSScriptRoot -Parent) "assembler\\Release\\vmap_assembler.exe"

if (!(Test-Path $extractor)) {
    throw "Missing vmapextract_v2.exe at $extractor"
}

if (!(Test-Path $assembler)) {
    throw "Missing vmap_assembler.exe at $assembler"
}

Push-Location $ClientPath
try {
    & $extractor
    if ($LASTEXITCODE -ne 0) {
        throw "vmapextract_v2.exe failed with exit code $LASTEXITCODE"
    }

    & $assembler "Buildings" $OutputPath
    if ($LASTEXITCODE -ne 0) {
        throw "vmap_assembler.exe failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
