# Merkel Rex — Cryptocurrency Trading Platform (Simulator)

A C++ order-book simulator. It loads a real 35-second snapshot of exchange data
from 17 March 2020, lets you place bids and asks against that market, runs a
matching engine at each time step, and exports the result to an HTML dashboard.

Built on the University of London / Coursera OOP specialisation.

---

## Contents

| File | What it is | Needed to run? |
|---|---|---|
| `main.cpp` | The simulator — all classes and the menu loop | Yes |
| `20200317.csv` | The market data: 3,540 orders, 5 products, 8 snapshots | Yes |
| `dashboard.html` | The visual dashboard | For the dashboard |
| `dashboard_data.js` | Data written by the app, read by the dashboard | Generated |
| `tests.cpp` | 37 unit tests | For testing |
| `demo_input.txt` | A scripted session, for an unattended demo | For the demo |
| `run_demo.bat` | Windows: build, test, demo, open dashboard | Convenience |
| `run_demo.ps1` | PowerShell version of the same | Convenience |
| `run_demo.sh` | macOS / Linux version of the same | Convenience |
| `demo_output.txt` | Transcript of the scripted session | Generated |
| `DEMO.md` | Annotated walkthrough with expected output | Reference |

**Keep every file in one folder.** The program looks for `20200317.csv` next to
itself, and `dashboard.html` looks for `dashboard_data.js` next to itself.

---

## Before you start: do you have a compiler?

Open a terminal in the project folder and run:

```
g++ --version
```

If you see a version number, you're ready. If you see "not recognized" or
"command not found", install one:

- **Windows** — install [MSYS2](https://www.msys2.org/), then run
  `pacman -S mingw-w64-ucrt-x86_64-gcc` in the MSYS2 terminal. Add
  `C:\msys64\ucrt64\bin` to your PATH and reopen your terminal.
  A simpler alternative is [WinLibs](https://winlibs.com/) — unzip it and add
  its `bin` folder to PATH.
- **macOS** — `xcode-select --install`
- **Linux** — `sudo apt install g++`

Anything supporting C++17 works. Developed against GCC 13.

---

## Quick start

### Windows

In File Explorer, double-click **`run_demo.bat`**. Or from a terminal:

```
.\run_demo.bat
```

The leading `.\` is required in PowerShell — it will not run a script in the
current folder without it.

### macOS / Linux

```bash
chmod +x run_demo.sh
./run_demo.sh
```

Either way this builds both programs, runs the tests, replays a scripted
trading session into `demo_output.txt`, and opens the dashboard.

---

## Running it yourself

### Build and run

Windows:

```
g++ -o trader.exe main.cpp -std=c++17
.\trader.exe
```

macOS / Linux:

```bash
g++ -o trader main.cpp -std=c++17
./trader
```

### The menu

```
1: Print help
2: Print exchange stats      best bid, best ask and spread, per product
3: Make an offer (Sell)      place an ask
4: Make a bid (Buy)          place a bid
5: Print wallet              current balances
6: Continue (Next Time Step) run the matching engine, advance 5 seconds
7: Export dashboard          write dashboard_data.js
8: Exit
```

### Placing an order

Options 3 and 4 expect `product,price,amount` on one line:

```
ETH/BTC,0.03,2.0
```

That bids 0.03 BTC each for 2 ETH. Valid products are `BTC/USDT`, `DOGE/BTC`,
`DOGE/USDT`, `ETH/BTC` and `ETH/USDT`.

Your order sits in the book until you choose **6**, which runs the matching
engine. A bid only fills if it is priced at or above someone's ask.

You start with 10 BTC, 100 ETH and 100,000 USDT.

### A session that actually trades

```
2                      look at the market
4                      make a bid
ETH/BTC,0.03,2.0       priced well above the best ask, so it must fill
6                      advance time - watch it fill
5                      see the wallet change
7                      export the dashboard
8                      exit
```

---

## The dashboard

1. Run the app and choose **7**. It writes `dashboard_data.js`.
2. Open `dashboard.html` — double-clicking it is fine.

The dashboard computes nothing itself. Every number in it was calculated by the
C++ engine and written out by the exporter; the page only draws what it is
given.

What you can do there: switch between the five products, drag the timeline
through all eight snapshots, read the depth ladder (asks above, bids below, the
spread as the gap between them), and see your own fills and wallet history.

If it says "No exported data yet", you skipped step 1.

---

## Tests

Windows:

```
g++ -o tests.exe tests.cpp -std=c++17
.\tests.exe
```

macOS / Linux:

```bash
g++ -o tests tests.cpp -std=c++17
./tests
```

Expect `37 / 37 checks passed`. Exits 0 on success, 1 on failure.

`tests.cpp` includes `main.cpp` directly and defines `UNIT_TESTS`, which
compiles out the app's `main()` so the harness can supply its own. That keeps
the project to a single source file.

---

## Troubleshooting

**`CSVReader: could not find 20200317.csv`**
The CSV is not in the folder you ran from. The program checks the working
directory, `..`, `../..` and `./data/`. If you build in an IDE, its working
directory is often not the project root — check its run configuration, or copy
the CSV next to the executable. Without it the app falls back to five mock
orders and prints a warning.

**`./run_demo.sh` does nothing on Windows**
PowerShell cannot execute `.sh` files. Windows opens them in whatever program is
associated with `.sh` — often VS Code. Use `run_demo.bat` instead.

**`.\run_demo.ps1` is blocked by execution policy**
Either run `powershell -ExecutionPolicy Bypass -File .\run_demo.ps1`, or just
use `run_demo.bat`, which is not subject to the policy.

**`trader.exe < demo_input.txt` fails in PowerShell**
PowerShell reserves `<` and has never implemented it. Pipe instead:

```powershell
Get-Content demo_input.txt | .\trader.exe
```

The `<` form works in `cmd.exe`, which is why `run_demo.bat` can use it.

**`g++ is not recognized`**
No compiler on PATH — see "Before you start" above.

**The dashboard is blank or shows the empty state**
Confirm `dashboard_data.js` is in the same folder as `dashboard.html`, and that
you chose option 7 before exiting.

---

## How it fits together

- `OrderBookEntry` — one bid or ask, plus the comparators used for sorting.
- `CSVReader` — tokenising and parsing; skips malformed lines instead of failing.
- `Wallet` — balances, affordability checks, and settlement after a fill.
- `OrderBook` — holds every entry, filters by product / time / type, and runs
  the matching engine. Filled orders are consumed and erased, so the book cannot
  end up crossing itself.
- `DashboardExporter` — walks every product at every timestamp and writes the
  results as JavaScript.
- `MerkelMain` — the menu loop and time-step simulation.

## About the data

`20200317.csv` is 3,540 orders across 5 products at 8 timestamps, five seconds
apart, from 17:01:24 to 17:02:00 on 17 March 2020 — days after the COVID crash,
which is why BTC sits near $5,350. Columns are
`timestamp,product,type,price,amount`.

Two things in the data are worth knowing, because they look like bugs and are
not. The recorded market never crosses itself, so matching the dataset against
itself produces zero sales — only orders you place can execute. And the final
snapshot has no orders at all for three of the five products, so the dashboard
shows gaps there rather than zeros.
