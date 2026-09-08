@echo off
REM run_demo.bat - builds the project, runs the test suite, and records a demo session.
REM Usage (from PowerShell or cmd):  .\run_demo.bat
REM Or just double-click it in Explorer.

echo ==============================================
echo  STEP 1/3  Building
echo ==============================================
g++ -o trader.exe main.cpp -std=c++17 -Wall
if errorlevel 1 goto :failed
g++ -o tests.exe tests.cpp -std=c++17 -Wall
if errorlevel 1 goto :failed
echo Build succeeded with no warnings.

echo.
echo ==============================================
echo  STEP 2/3  Unit tests
echo ==============================================
tests.exe
if errorlevel 1 echo (Some tests did not pass - see above.)

echo.
echo ==============================================
echo  STEP 3/3  Recorded demo session
echo ==============================================
echo Feeding demo_input.txt into the app, saving to demo_output.txt
trader.exe < demo_input.txt > demo_output.txt
echo Done. Transcript written to demo_output.txt

echo.
echo Highlights from the session:
findstr /C:"CSVReader:" /C:"TRADE FILLED" /C:"insufficient" /C:"Bad input" /C:"Spread" /C:"BTC " /C:"ETH " /C:"USDT " demo_output.txt

echo.
echo ==============================================
echo  Dashboard
echo ==============================================
if exist dashboard_data.js (
  echo Opening dashboard.html in your browser...
  start "" dashboard.html
) else (
  echo dashboard_data.js was not created - run trader.exe and choose option 7.
)

echo.
echo Demo complete.
pause
goto :eof

:failed
echo.
echo BUILD FAILED - fix the errors above and try again.
pause
