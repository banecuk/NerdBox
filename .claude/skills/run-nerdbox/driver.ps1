#requires -version 5.1
<#
Driver for building, flashing, and talking to the real NerdBox device over
its Wi-Fi HTTP API. Windows/PowerShell only — matches the project (COM
ports, esptool). Run from anywhere; paths below are resolved from the repo
root regardless of caller's cwd.

Usage:
  .\driver.ps1 build                      # both firmware envs
  .\driver.ps1 flash [-Port COM5]         # build + upload debug env
  .\driver.ps1 ip [-Mac 3C:84:27:13:A7:3C] # discover device IP via ARP
  .\driver.ps1 status [-DeviceIp x.x.x.x]
  .\driver.ps1 pc      [-DeviceIp x.x.x.x]
  .\driver.ps1 logs    [-DeviceIp x.x.x.x]
  .\driver.ps1 config  [-DeviceIp x.x.x.x]
  .\driver.ps1 screen <main|settings|game|weather> [-DeviceIp x.x.x.x]
  .\driver.ps1 restart [-DeviceIp x.x.x.x]
  .\driver.ps1 test                       # native host-side unit tests
  .\driver.ps1 monitor [-Port COM5]       # attach serial monitor (blocks)

If -DeviceIp is omitted, the device's IP is auto-discovered from the ARP
table by matching the serial number PlatformIO reports for the connected
device (which is its Wi-Fi MAC on this board) — see the `ip` command.
#>
param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet('build', 'flash', 'ip', 'status', 'pc', 'logs', 'config', 'screen', 'restart', 'test', 'monitor')]
    [string]$Command,

    [Parameter(Position = 1)]
    [string]$Screen,

    [string]$Port,
    [string]$Mac,
    [string]$DeviceIp
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))

function Get-Pio {
    $cmd = Get-Command pio -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $fallback = Join-Path $HOME '.platformio\penv\Scripts\pio.exe'
    if (Test-Path $fallback) { return $fallback }
    throw "pio not found on PATH or at $fallback"
}

function Get-DeviceMac {
    # `pio device list` reports the connected board's serial number as
    # SER=xx:xx:xx:xx:xx:xx — on this board (native USB CDC) that value IS
    # the Wi-Fi MAC address, so it doubles as the ARP lookup key.
    $pio = Get-Pio
    $listing = & $pio device list 2>&1 | Out-String
    $match = [regex]::Match($listing, 'SER=([0-9A-Fa-f:]{17})')
    if (-not $match.Success) {
        throw "No USB-serial device found (pio device list showed no SER=... entry). Is the board plugged in?"
    }
    return $match.Groups[1].Value
}

function Resolve-DeviceIp {
    if ($DeviceIp) { return $DeviceIp }
    $mac = if ($Mac) { $Mac } else { Get-DeviceMac }
    $normalized = ($mac -replace ':', '-').ToUpper()
    $neighbor = Get-NetNeighbor -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.LinkLayerAddress -eq $normalized }
    if (-not $neighbor) {
        throw "No ARP entry for MAC $mac. Ping the device's subnet broadcast first (or pass -DeviceIp), then retry."
    }
    return ($neighbor | Select-Object -First 1 -ExpandProperty IPAddress)
}

switch ($Command) {
    'build' {
        $pio = Get-Pio
        & $pio run -d $RepoRoot -e WT32-SC01-PLUS-debug
        & $pio run -d $RepoRoot -e WT32-SC01-PLUS-release
        break
    }

    'flash' {
        $pio = Get-Pio
        $args = @('run', '-d', $RepoRoot, '-e', 'WT32-SC01-PLUS-debug', '--target', 'upload')
        if ($Port) { $args += @('--upload-port', $Port) }
        & $pio @args
        break
    }

    'ip' {
        Write-Output (Resolve-DeviceIp)
        break
    }

    'status' {
        $ip = Resolve-DeviceIp
        Invoke-RestMethod -Uri "http://$ip/api/status" -TimeoutSec 5 | ConvertTo-Json -Depth 6
        break
    }

    'pc' {
        $ip = Resolve-DeviceIp
        Invoke-RestMethod -Uri "http://$ip/api/pc" -TimeoutSec 5 | ConvertTo-Json -Depth 6
        break
    }

    'logs' {
        $ip = Resolve-DeviceIp
        (Invoke-WebRequest -Uri "http://$ip/logs" -TimeoutSec 5).Content
        break
    }

    'config' {
        $ip = Resolve-DeviceIp
        (Invoke-WebRequest -Uri "http://$ip/config" -TimeoutSec 5).Content
        break
    }

    'screen' {
        if (-not $Screen) { throw "Usage: .\driver.ps1 screen <main|settings|game|weather>" }
        $valid = @('main', 'settings', 'game', 'weather')
        if ($valid -notcontains $Screen) { throw "Screen must be one of: $($valid -join ', ')" }
        $ip = Resolve-DeviceIp
        Invoke-RestMethod -Uri "http://$ip/screen/$Screen" -Method Post -TimeoutSec 5
        Start-Sleep -Milliseconds 500
        $current = Invoke-RestMethod -Uri "http://$ip/api/status" -TimeoutSec 5
        Write-Output "Device now on screen: $($current.ui.screen)"
        break
    }

    'restart' {
        $ip = Resolve-DeviceIp
        Invoke-RestMethod -Uri "http://$ip/restart" -Method Post -TimeoutSec 5
        break
    }

    'test' {
        $pio = Get-Pio
        & $pio test -d $RepoRoot -e native
        break
    }

    'monitor' {
        $pio = Get-Pio
        $args = @('device', 'monitor', '--baud', '115200')
        if ($Port) { $args += @('--port', $Port) }
        & $pio @args
        break
    }
}
