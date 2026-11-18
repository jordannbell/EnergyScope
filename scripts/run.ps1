# Starts the server in the background, opens the MFC client, and stops the
# server when the client window is closed.
# Usage:  .\scripts\run.ps1        (build first with .\scripts\build.ps1)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$server = "$root\build\server\energy_server.exe"
$client = "$root\build\app\energy_client.exe"

foreach ($exe in $server, $client) {
    if (-not (Test-Path $exe)) { throw "$exe not found. Run .\scripts\build.ps1 first." }
}

$serverProcess = Start-Process $server -ArgumentList 5555, "$root\energy.sqlite" `
    -PassThru -WindowStyle Minimized
try {
    Start-Process $client -Wait
} finally {
    Stop-Process -Id $serverProcess.Id -ErrorAction SilentlyContinue
}
