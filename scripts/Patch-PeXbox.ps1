# Patch PE32 optional header for Xbox imagebld (subsystem 14, 1 MiB stack).
param(
    [Parameter(Mandatory)]
    [string]$Path
)

$ErrorActionPreference = 'Stop'
$pathFull = [IO.Path]::GetFullPath($Path)
if (-not (Test-Path -LiteralPath $pathFull)) {
    throw "PE not found: $pathFull"
}

$fs = [IO.File]::Open($pathFull, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try {
    $len = $fs.Length
    if ($len -lt 0x200) { throw "File too small to be a PE: $pathFull" }

    $buf = New-Object byte[] $len
    [void]$fs.Read($buf, 0, $len)

    $e_lfanew = [BitConverter]::ToInt32($buf, 0x3C)
    if ($e_lfanew -lt 0 -or ($e_lfanew + 0x100) -gt $len) { throw "Invalid PE offset in $pathFull" }

    $peOff = $e_lfanew
    if ([Text.Encoding]::ASCII.GetString($buf, $peOff, 4) -ne "PE`0`0") {
        throw "Missing PE signature in $pathFull"
    }

    $optOff = $peOff + 4 + 20
    $magic = [BitConverter]::ToUInt16($buf, $optOff)
    if ($magic -ne 0x10B) { throw "Expected PE32 image in $pathFull" }

    [BitConverter]::GetBytes([uint16]4).CopyTo($buf, $optOff + 40)   # MajorOperatingSystemVersion
    [BitConverter]::GetBytes([uint16]0).CopyTo($buf, $optOff + 42)   # MinorOperatingSystemVersion
    [BitConverter]::GetBytes([uint16]1).CopyTo($buf, $optOff + 48)   # MajorSubsystemVersion
    [BitConverter]::GetBytes([uint16]0).CopyTo($buf, $optOff + 50)   # MinorSubsystemVersion
    [BitConverter]::GetBytes([uint32]0x00100000).CopyTo($buf, $optOff + 72) # SizeOfStackReserve (TriangleXDK)
    [BitConverter]::GetBytes([uint32]0x00001000).CopyTo($buf, $optOff + 76) # SizeOfStackCommit (TriangleXDK)
    [BitConverter]::GetBytes([uint16]14).CopyTo($buf, $optOff + 68)    # IMAGE_SUBSYSTEM_XBOX
    [BitConverter]::GetBytes([uint16]0).CopyTo($buf, $optOff + 70)   # DllCharacteristics

    $fs.Seek(0, [IO.SeekOrigin]::Begin) | Out-Null
    $fs.Write($buf, 0, $len)
    $fs.SetLength($len)
}
finally {
    $fs.Dispose()
}

Write-Host "Patched Xbox PE headers: $pathFull"
