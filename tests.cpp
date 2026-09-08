/**
 * tests.cpp - a tiny dependency-free test harness for the trading platform.
 *
 * Build:  g++ -o tests tests.cpp -std=c++17 -DUNIT_TESTS
 * Run:    ./tests
 *
 * It includes main.cpp directly (with UNIT_TESTS defined so that the app's
 * own main() is compiled out) and exercises each class in isolation.
 */

#ifndef UNIT_TESTS
#define UNIT_TESTS
#endif
#include "main.cpp"

#include <cmath>

static int testsRun = 0;
static int testsFailed = 0;

void check(const std::string& name, bool condition) {
    ++testsRun;
    if (condition) {
        std::cout << "  [PASS] " << name << std::endl;
    } else {
        ++testsFailed;
        std::cout << "  [FAIL] " << name << std::endl;
    }
}

bool nearly(double a, double b, double tol = 1e-9) {
    return std::fabs(a - b) < tol;
}

void testCSVReader() {
    std::cout << "\nCSVReader" << std::endl;

    std::vector<std::string> t = CSVReader::tokenise("a,b,c", ',');
    check("tokenise splits into 3 tokens", t.size() == 3);
    check("tokenise keeps order", t[0] == "a" && t[2] == "c");

    std::vector<std::string> pair = CSVReader::tokenise("ETH/BTC", '/');
    check("tokenise splits a product pair", pair.size() == 2 && pair[1] == "BTC");

    OrderBookEntry e = CSVReader::stringsToOBE(
        CSVReader::tokenise("2020/03/17 17:01:24.884492,ETH/BTC,bid,0.02187308,7.44564869", ','));
    check("parses price", nearly(e.price, 0.02187308));
    check("parses amount", nearly(e.amount, 7.44564869));
    check("parses product", e.product == "ETH/BTC");
    check("parses order type", e.orderType == OrderBookType::bid);

    bool threw = false;
    try { CSVReader::stringsToOBE(CSVReader::tokenise("only,three,columns", ',')); }
    catch (const std::exception&) { threw = true; }
    check("rejects a malformed line", threw);
}

void testWallet() {
    std::cout << "\nWallet" << std::endl;

    Wallet w;
    w.insertCurrency("BTC", 10);
    check("holds what was inserted", w.containsCurrency("BTC", 10));
    check("does not hold more than inserted", !w.containsCurrency("BTC", 10.1));
    check("does not hold unknown currency", !w.containsCurrency("ETH", 0.1));

    check("removes an affordable amount", w.removeCurrency("BTC", 4));
    check("balance reduced correctly", w.containsCurrency("BTC", 6) && !w.containsCurrency("BTC", 6.1));
    check("refuses to overdraw", !w.removeCurrency("BTC", 100));

    bool threw = false;
    try { w.insertCurrency("BTC", -5); } catch (const std::exception&) { threw = true; }
    check("rejects a negative deposit", threw);

    // Affordability: to bid 2 ETH at 0.02 BTC each we need 0.04 BTC.
    Wallet w2;
    w2.insertCurrency("BTC", 0.05);
    OrderBookEntry bid{0.02, 2.0, "t", "ETH/BTC", OrderBookType::bid, "simuser"};
    check("can afford a bid it has funds for", w2.canFulfillOrder(bid));

    OrderBookEntry bigBid{0.02, 200.0, "t", "ETH/BTC", OrderBookType::bid, "simuser"};
    check("rejects a bid it cannot fund", !w2.canFulfillOrder(bigBid));

    OrderBookEntry ask{0.02, 1.0, "t", "ETH/BTC", OrderBookType::ask, "simuser"};
    check("rejects an ask with no ETH to sell", !w2.canFulfillOrder(ask));

    // Settlement: buying 2 ETH at 0.02 costs 0.04 BTC.
    Wallet w3;
    w3.insertCurrency("BTC", 1.0);
    w3.insertCurrency("ETH", 0.0);
    OrderBookEntry fill{0.02, 2.0, "t", "ETH/BTC", OrderBookType::bidsale, "simuser"};
    w3.processSale(fill);
    check("credits the bought asset", w3.containsCurrency("ETH", 2.0) && !w3.containsCurrency("ETH", 2.01));
    check("debits the paid asset", w3.containsCurrency("BTC", 0.96) && !w3.containsCurrency("BTC", 0.9601));
}

// The OrderBook and matching-engine tests need the real dataset. Checked once
// up front so a missing file gives one clear message instead of a cascade.
bool datasetAvailable() {
    std::ifstream f{"20200317.csv"};
    if (f.is_open()) return true;
    std::ifstream f2{"../20200317.csv"};
    return f2.is_open();
}

void testOrderBook() {
    std::cout << "\nOrderBook" << std::endl;

    OrderBook book{"20200317.csv"};

    std::vector<std::string> products = book.getKnownProducts();
    check("finds 5 products in the dataset", products.size() == 5);
    check("products are sorted and include ETH/BTC",
          std::find(products.begin(), products.end(), "ETH/BTC") != products.end());

    std::string t0 = book.getEarliestTime();
    check("earliest time is the first timestamp", t0 == "2020/03/17 17:01:24.884492");

    std::string t1 = book.getNextTime(t0);
    check("next time moves forward", t1 > t0);

    std::vector<OrderBookEntry> asks = book.getOrders(OrderBookType::ask, "ETH/BTC", t0);
    check("filters asks for one product at one timestamp", asks.size() == 50);
    for (OrderBookEntry& e : asks) {
        if (e.product != "ETH/BTC" || e.timestamp != t0 || e.orderType != OrderBookType::ask) {
            check("every filtered entry matches the filter", false);
            return;
        }
    }
    check("every filtered entry matches the filter", true);

    check("high price is >= low price",
          OrderBook::getHighPrice(asks) >= OrderBook::getLowPrice(asks));

    // A real order book should not cross itself: best bid < best ask.
    std::vector<OrderBookEntry> bids = book.getOrders(OrderBookType::bid, "ETH/BTC", t0);
    check("market has a positive spread",
          OrderBook::getLowPrice(asks) > OrderBook::getHighPrice(bids));
    check("dataset alone produces no sales", book.matchAsksToBids("ETH/BTC", t0).empty());
}

void testMatchingEngine() {
    std::cout << "\nMatching engine" << std::endl;

    OrderBook book{"20200317.csv"};
    std::string t0 = book.getEarliestTime();

    std::vector<OrderBookEntry> asks = book.getOrders(OrderBookType::ask, "ETH/BTC", t0);
    double bestAsk = OrderBook::getLowPrice(asks);

    // Place an aggressive bid, priced above the best ask, so it must match.
    OrderBookEntry myBid{bestAsk * 1.5, 2.0, t0, "ETH/BTC", OrderBookType::bid, "simuser"};
    book.insertOrder(myBid);

    std::vector<OrderBookEntry> sales = book.matchAsksToBids("ETH/BTC", t0);
    check("an aggressive bid produces a sale", sales.size() >= 1);
    if (sales.empty()) return;

    check("sale is flagged as ours", sales[0].username == "simuser");
    check("sale is recorded as a purchase", sales[0].orderType == OrderBookType::bidsale);
    check("buyer pays the ask price, not their own", nearly(sales[0].price, bestAsk));

    double totalMatched = 0;
    for (OrderBookEntry& s : sales) totalMatched += s.amount;
    check("filled amount never exceeds the order", totalMatched <= 2.0 + 1e-9);

    // A filled order must leave the book, otherwise it lingers as a phantom
    // best bid and the book appears to cross itself.
    std::vector<OrderBookEntry> bidsAfter = book.getOrders(OrderBookType::bid, "ETH/BTC", t0);
    double phantom = 0;
    for (OrderBookEntry& e : bidsAfter) if (e.username == "simuser") phantom += e.amount;
    check("our filled bid is consumed", nearly(phantom, 2.0 - totalMatched, 1e-9));

    std::vector<OrderBookEntry> asksAfter = book.getOrders(OrderBookType::ask, "ETH/BTC", t0);
    check("book does not cross itself after matching",
          OrderBook::getLowPrice(asksAfter) > OrderBook::getHighPrice(bidsAfter));
    check("no zero-amount orders remain",
          std::none_of(asksAfter.begin(), asksAfter.end(),
                       [](const OrderBookEntry& e) { return e.amount <= 0; }));
}

int main() {
    std::cout << "Running tests for the Merkel Rex trading platform" << std::endl;

    testCSVReader();
    testWallet();

    if (datasetAvailable()) {
        testOrderBook();
        testMatchingEngine();
    } else {
        std::cout << "\n*** SKIPPED: OrderBook and matching-engine tests ***" << std::endl;
        std::cout << "    20200317.csv is not in this folder, so there is no market" << std::endl;
        std::cout << "    data to test against. Copy it next to tests.exe and rerun." << std::endl;
        ++testsFailed;
        ++testsRun;
    }

    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << testsRun - testsFailed << " / " << testsRun << " checks passed" << std::endl;
    std::cout << (testsFailed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    return testsFailed == 0 ? 0 : 1;
}
