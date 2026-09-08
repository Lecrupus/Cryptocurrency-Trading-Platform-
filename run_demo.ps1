# run_demo.ps1 - builds the project, runs the test suite, and records a demo session.
#
# Usage:  .\run_demo.ps1
# If PowerShell blocks it with an execution-policy error, either run
#   powershell -ExecutionPolicy Bypass -File .\run_demo.ps1
# or just use run_demo.bat instead.

Write-Host "=============================================="
Write-Host " STEP 1/3  Building"
Write-Host "=============================================="

g++ -o trader.exe main.cpp -std=c++17 -Wall
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED" -ForegroundColor Red; exit 1 }

g++ -o tests.exe tests.cpp -std=c++17 -Wall
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED" -ForegroundColor Red; exit 1 }

Write-Host "Build succeeded with no warnings." -ForegroundColor Green

Write-Host ""
Write-Host "=============================================="
Write-Host " STEP 2/3  Unit tests"
Write-Host "=============================================="

.\tests.exe
if ($LASTEXITCODE -ne 0) { Write-Host "(Some tests did not pass - see above.)" -ForegroundColor Yellow }

Write-Host ""
Write-Host "=============================================="
Write-Host " STEP 3/3  Recorded demo session"
Write-Host "=============================================="
Write-Host "Feeding demo_input.txt into the app, saving to demo_output.txt"

# PowerShell has no '<' input redirection, so pipe the file in instead.
Get-Content demo_input.txt | .\trader.exe | Out-File -Encoding utf8 demo_output.txt

$lines = (Get-Content demo_output.txt).Count
Write-Host "Done. $lines lines written to demo_output.txt"

Write-Host ""
Write-Host "Highlights from the session:"
Select-String -Path demo_output.txt `
    -Pattern "CSVReader:|TRADE FILLED|insufficient|Bad input|Spread|BTC  |ETH  |USDT " |
    Select-Object -First 30 |
    ForEach-Object { $_.Line }

Write-Host ""
Write-Host "=============================================="
Write-Host " Dashboard"
Write-Host "=============================================="
if (Test-Path dashboard_data.js) {
    Write-Host "Opening dashboard.html in your browser..."
    Invoke-Item dashboard.html
} else {
    Write-Host "dashboard_data.js was not created - run trader.exe and choose option 7." -ForegroundColor Yellow
}
