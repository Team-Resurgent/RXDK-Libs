# Reports which COFF archives define a given symbol, by reading the archive's
# first linker member (the "/" symbol index), which lists defined symbols only.
# Used to check that no SDK library exports the global C++ allocation operators.
param(
    [Parameter(Mandatory = $true)][string[]] $Symbol,
    [Parameter(Mandatory = $true)][string] $LibDir
)

function Get-ArchiveDefinedSymbols([string] $path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -lt 68) { return @() }
    # "!<arch>\n" then a 60-byte member header whose size field is at offset 48.
    $size = [int]("" + [Text.Encoding]::ASCII.GetString($bytes, 8 + 48, 10)).Trim()
    $start = 68
    $names = New-Object System.Collections.Generic.List[string]
    # Header: 4-byte BE count, then count * 4-byte BE offsets, then the names.
    $count = [BitConverter]::ToUInt32(($bytes[($start + 3)..$start]), 0)
    $p = $start + 4 + ($count * 4)
    $end = $start + $size
    $sb = New-Object Text.StringBuilder
    while ($p -lt $end) {
        if ($bytes[$p] -eq 0) {
            if ($sb.Length -gt 0) { $names.Add($sb.ToString()) | Out-Null; $sb.Clear() | Out-Null }
        } else {
            $sb.Append([char]$bytes[$p]) | Out-Null
        }
        $p++
    }
    return $names
}

foreach ($lib in Get-ChildItem $LibDir -Filter *.lib) {
    $defined = Get-ArchiveDefinedSymbols $lib.FullName
    $hits = $defined | Where-Object { $Symbol -contains $_ }
    if ($hits) {
        "{0,-20} {1}" -f $lib.Name, (($hits | Sort-Object -Unique) -join ' ')
    }
}
