param(
    [int]$Port = 9123,
    [string]$RomPath = "output\N64FlashcartMenu.n64",
    [string]$AresPath = "",
    [string]$GdbPath = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($AresPath)) {
    $candidates = Get-ChildItem -Path "tools\ares" -Recurse -Filter "ares.exe" -ErrorAction SilentlyContinue
    if ($candidates.Count -gt 0) {
        $AresPath = $candidates[0].FullName
    }
}

if (-not (Test-Path $AresPath)) {
    throw "Ares executable not found. Pass -AresPath or extract Ares under tools\ares\."
}

try {
    Unblock-File -Path $AresPath -ErrorAction Stop
} catch {
    # Best effort: some systems may not set Zone.Identifier or may deny unblock.
}

if (-not (Test-Path $RomPath)) {
    throw "ROM not found at '$RomPath'. Build first with make all in the devcontainer."
}

Write-Host "Starting Ares in GDB wait mode..."
$proc = Start-Process -FilePath $AresPath -ArgumentList @(
    "--no-file-prompt",
    "--setting", "General/HomebrewMode=true",
    "--setting", "DebugServer/Enabled=true",
    "--setting", "DebugServer/UseIPv4=true",
    "--setting", "DebugServer/Port=$Port",
    "--setting", "Boot/AwaitGDBClient=true",
    $RomPath
) -PassThru
$sessionStart = Get-Date

try {
    $connected = $false
    $tcp = $null
    for ($i = 0; $i -lt 25; $i++) {
        Start-Sleep -Milliseconds 200
        $tcp = New-Object System.Net.Sockets.TcpClient
        try {
            $tcp.Connect("127.0.0.1", $Port)
            $connected = $true
            break
        } catch {
            $tcp.Dispose()
        }
    }

    if (-not $connected) {
        throw "GDB server did not open on 127.0.0.1:$Port"
    }

    Write-Host "Ares debug server is reachable on 127.0.0.1:$Port"

    if ([string]::IsNullOrWhiteSpace($GdbPath)) {
        $gdbCandidates = @(
            (Get-Command gdb.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source),
            (Get-Command gdb-multiarch.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source)
        ) | Where-Object { $_ -and (Test-Path $_) }

        if ($gdbCandidates.Count -gt 0) {
            $GdbPath = $gdbCandidates[0]
        }
    }

    if ([string]::IsNullOrWhiteSpace($GdbPath) -or -not (Test-Path $GdbPath)) {
        Write-Host "GDB binary not found on host PATH; protocol attach step skipped."
        Write-Host "GDB smoke PASS (server readiness confirmed)."
        return
    }

    Write-Host "Running GDB attach via: $GdbPath"
    $gdbOut = & $GdbPath -q -batch `
        -ex "set architecture mips:4300" `
        -ex "file build/N64FlashcartMenu.elf" `
        -ex "target remote 127.0.0.1:$Port" `
        -ex "info registers pc sp ra" `
        -ex "detach" `
        2>&1

    $joined = ($gdbOut | Out-String)
    if ($joined -notmatch "Remote debugging using") {
        throw "GDB attach did not succeed. Output:`n$joined"
    }

    Write-Host "GDB smoke PASS (real client attach successful)."
    Write-Host $joined
} finally {
    if ($tcp) { $tcp.Dispose() }
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }

    # Ares may spawn helper processes. Stop any instance created during this smoke run.
    Get-Process ares -ErrorAction SilentlyContinue |
        Where-Object { $_.StartTime -ge $sessionStart.AddSeconds(-2) } |
        ForEach-Object {
            Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
        }
}
