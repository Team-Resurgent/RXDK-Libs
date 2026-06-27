# Patch PE32 optional header for Xbox imagebld (subsystem 14, large stack).
# The PE SizeOfStackCommit is the field the kit boot path actually uses for the
# main-thread stack (xbdm dmmodule.c: pxbe->StackSize = PE SizeOfStackCommit),
# so it must match the stack size passed to imagebld. The caller threads a
# single $StackCommit value through both to keep them in sync.
param(
    [Parameter(Mandatory)]
    [string]$Path,
    [uint32]$StackReserve = 0x00100000,
    [uint32]$StackCommit = 0x00010000
)

$ErrorActionPreference = 'Stop'

function Get-SectionNameForRva {
    param(
        [byte[]]$Buf,
        [int]$PeOff,
        [uint32]$Rva
    )
    $fileHeaderOff = $PeOff + 4
    $numSections = [BitConverter]::ToUInt16($Buf, $fileHeaderOff + 2)
    $sizeOptional = [BitConverter]::ToUInt16($Buf, $fileHeaderOff + 16)
    $optOff = $PeOff + 4 + 20
    $sectionOff = $optOff + $sizeOptional
    for ($s = 0; $s -lt $numSections; $s++) {
        $secOff = $sectionOff + ($s * 40)
        $va = [BitConverter]::ToUInt32($Buf, $secOff + 12)
        $virtSize = [BitConverter]::ToUInt32($Buf, $secOff + 8)
        if ($Rva -ge $va -and $Rva -lt ($va + $virtSize)) {
            return [Text.Encoding]::ASCII.GetString($Buf, $secOff, 8).Trim([char]0)
        }
    }
    return $null
}

function Find-TlsDirectoryRva {
    param(
        [byte[]]$Buf,
        [int]$PeOff
    )
    $optOff = $PeOff + 4 + 20
    $imageBase = [BitConverter]::ToUInt32($Buf, $optOff + 0x1c)
    $len = $Buf.Length
    for ($i = 0; $i -le ($len - 24); $i += 4) {
        $start = [BitConverter]::ToUInt32($Buf, $i)
        $end = [BitConverter]::ToUInt32($Buf, $i + 4)
        $index = [BitConverter]::ToUInt32($Buf, $i + 8)
        $callbacks = [BitConverter]::ToUInt32($Buf, $i + 12)
        $zeroFill = [BitConverter]::ToUInt32($Buf, $i + 16)
        $characteristics = [BitConverter]::ToUInt32($Buf, $i + 20)
        if ($callbacks -ne 0) { continue }
        if ($zeroFill -ne 0) { continue }
        if ($start -lt $imageBase -or $end -le $start) { continue }
        if (($end - $start) -gt 0x2000) { continue }
        if ($index -lt $imageBase) { continue }
        $tlsDirRva = Find-RvaForFileOffset -Buf $Buf -PeOff $PeOff -FileOffset $i
        if ($null -eq $tlsDirRva) { continue }
        $secName = Get-SectionNameForRva -Buf $Buf -PeOff $PeOff -Rva $tlsDirRva
        if ($secName -notlike '.rdata*') { continue }
        $startRva = $start - $imageBase
        $tlsSec = Get-SectionNameForRva -Buf $Buf -PeOff $PeOff -Rva $startRva
        if ($tlsSec -ne '.tls') { continue }
        return $tlsDirRva
    }
    return $null
}

function Find-RvaForFileOffset {
    param(
        [byte[]]$Buf,
        [int]$PeOff,
        [int]$FileOffset
    )
    $fileHeaderOff = $PeOff + 4
    $numSections = [BitConverter]::ToUInt16($Buf, $fileHeaderOff + 2)
    $sizeOptional = [BitConverter]::ToUInt16($Buf, $fileHeaderOff + 16)
    $optOff = $PeOff + 4 + 20
    $sectionOff = $optOff + $sizeOptional
    for ($s = 0; $s -lt $numSections; $s++) {
        $secOff = $sectionOff + ($s * 40)
        $va = [BitConverter]::ToUInt32($Buf, $secOff + 12)
        $rawSize = [BitConverter]::ToUInt32($Buf, $secOff + 16)
        $rawPtr = [BitConverter]::ToUInt32($Buf, $secOff + 20)
        if ($rawSize -eq 0) { continue }
        if ($FileOffset -ge $rawPtr -and $FileOffset -lt ($rawPtr + $rawSize)) {
            return [uint32]($va + ($FileOffset - $rawPtr))
        }
    }
    return $null
}

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
    [BitConverter]::GetBytes($StackReserve).CopyTo($buf, $optOff + 72) # SizeOfStackReserve
    # SizeOfStackCommit is the REAL main-thread stack on the kit: xbdm sets the
    # XBE StackSize from this field (kernel dmmodule.c). 4 KiB is far too small
    # for the DWARF C++ exception unwinder (it overflowed mid-unwind). Threaded
    # from the caller so it matches imagebld's /stack value.
    [BitConverter]::GetBytes($StackCommit).CopyTo($buf, $optOff + 76) # SizeOfStackCommit
    [BitConverter]::GetBytes([uint32]0x00100000).CopyTo($buf, $optOff + 80) # SizeOfHeapReserve
    [BitConverter]::GetBytes([uint32]0x00010000).CopyTo($buf, $optOff + 84) # SizeOfHeapCommit (64 KiB initial)
    [BitConverter]::GetBytes([uint16]14).CopyTo($buf, $optOff + 68)    # IMAGE_SUBSYSTEM_XBOX
    [BitConverter]::GetBytes([uint16]0).CopyTo($buf, $optOff + 70)   # DllCharacteristics

    # lld can point IMAGE_DIRECTORY_ENTRY_TLS at LLVM metadata in .rdata instead of
    # our _tls_used (.rdata$T). imagebld needs the real IMAGE_TLS_DIRECTORY.
    $tlsDirRva = Find-TlsDirectoryRva -Buf $buf -PeOff $peOff
    if ($null -ne $tlsDirRva) {
        [BitConverter]::GetBytes([uint32]$tlsDirRva).CopyTo($buf, $optOff + 0xC0)
        [BitConverter]::GetBytes([uint32]0x18).CopyTo($buf, $optOff + 0xC4)
    }

    # Ensure .tls stays in the mapped image (clear IMAGE_SCN_MEM_DISCARDABLE).
    $fileHeaderOff = $peOff + 4
    $numSections = [BitConverter]::ToUInt16($buf, $fileHeaderOff + 2)
    $sizeOptional = [BitConverter]::ToUInt16($buf, $fileHeaderOff + 16)
    $sectionOff = $optOff + $sizeOptional
    for ($s = 0; $s -lt $numSections; $s++) {
        $secOff = $sectionOff + ($s * 40)
        $name = [Text.Encoding]::ASCII.GetString($buf, $secOff, 8).Trim([char]0)
        if ($name -ne '.tls') { continue }
        $charsOff = $secOff + 36
        $chars = [BitConverter]::ToUInt32($buf, $charsOff)
        $chars = $chars -band (-bnot [uint32]0x02000000)
        [BitConverter]::GetBytes([uint32]$chars).CopyTo($buf, $charsOff)
        break
    }

    $fs.Seek(0, [IO.SeekOrigin]::Begin) | Out-Null
    $fs.Write($buf, 0, $len)
    $fs.SetLength($len)
}
finally {
    $fs.Dispose()
}

Write-Host "Patched Xbox PE headers: $pathFull"
