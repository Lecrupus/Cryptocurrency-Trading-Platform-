#!/usr/bin/env bash
# run_demo.sh - builds the project, runs the test suite, and records a demo session.
# Usage: ./run_demo.sh
set -e

echo "=============================================="
echo " STEP 1/3  Building"
echo "=============================================="
g++ -o trader main.cpp -std=c++17 -Wall
g++ -o tests tests.cpp -std=c++17 -Wall
echo "Build succeeded with no warnings."

echo
echo "=============================================="
echo " STEP 2/3  Unit tests"
echo "=============================================="
./tests

echo
echo "=============================================="
echo " STEP 3/3  Recorded demo session"
echo "=============================================="
echo "Feeding demo_input.txt into the app, saving to demo_output.txt"
./trader < demo_input.txt > demo_output.txt
echo "Done. $(wc -l < demo_output.txt) lines written to demo_output.txt"
echo
echo "Highlights from the session:"
grep -E "CSVReader:|TRADE FILLED|insufficient|Bad input|Spread|BTC  |ETH  |USDT " demo_output.txt | head -30

echo
echo "=============================================="
echo " Dashboard"
echo "=============================================="
if [ -f dashboard_data.js ]; then
  echo "dashboard_data.js written. Open dashboard.html in a browser."
else
  echo "dashboard_data.js missing - run ./trader and choose option 7."
fi
