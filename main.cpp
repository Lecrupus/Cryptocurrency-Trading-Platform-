/**
 * Cryptocurrency Trading Platform (Simulator)
 * Based on University of London / Coursera OOP Specialization
 *
 * Features:
 * - OOP Architecture (Wallet, OrderBook, Matching Engine)
 * - STL Containers (Vectors, Maps)
 * - Loads real market data from CSV
 * - Time-step simulation
 * - Matching Engine
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

// ==========================================
// 1. Data Structures & Enums
// ==========================================

enum class OrderBookType { bid, ask, unknown, asksale, bidsale };

class OrderBookEntry {
public:
    double price;
    double amount;
    std::string timestamp;
    std::string product;
    OrderBookType orderType;
    std::string username;

    OrderBookEntry(double _price, double _amount, std::string _timestamp,
                   std::string _product, OrderBookType _orderType, std::string _username = "dataset")
    : price(_price), amount(_amount), timestamp(_timestamp),
      product(_product), orderType(_orderType), username(_username) {}

    static OrderBookType stringToOrderBookType(const std::string& s) {
        if (s == "ask") return OrderBookType::ask;
        if (s == "bid") return OrderBookType::bid;
        return OrderBookType::unknown;
    }

    static bool compareByTimestamp(const OrderBookEntry& e1, const OrderBookEntry& e2) {
        return e1.timestamp < e2.timestamp;
    }

    static bool compareByPriceAsc(const OrderBookEntry& e1, const OrderBookEntry& e2) {
        return e1.price < e2.price;
    }

    static bool compareByPriceDesc(const OrderBookEntry& e1, const OrderBookEntry& e2) {
        return e1.price > e2.price;
    }
};

// ==========================================
// 2. CSV / String Parsing Utilities
// ==========================================

class CSVReader {
public:
    static std::vector<std::string> tokenise(std::string csvLine, char separator) {
        std::vector<std::string> tokens;
        std::string::size_type start, end;
        std::string token;
        start = csvLine.find_first_not_of(separator, 0);
        do {
            end = csvLine.find_first_of(separator, start);
            if (start == csvLine.length() || start == end) break;
            if (end != std::string::npos) token = csvLine.substr(start, end - start);
            else token = csvLine.substr(start, csvLine.length() - start);
            tokens.push_back(token);
            start = end + 1;
        } while (end != std::string::npos);
        return tokens;
    }

    // Turns one line of CSV into an OrderBookEntry. Throws if the line is malformed.
    static OrderBookEntry stringsToOBE(std::vector<std::string> tokens) {
        if (tokens.size() != 5) throw std::invalid_argument("wrong number of columns");
        double price = std::stod(tokens[3]);
        double amount = std::stod(tokens[4]);
        return OrderBookEntry(price, amount, tokens[0], tokens[1],
                              OrderBookEntry::stringToOrderBookType(tokens[2]));
    }

    // Reads the whole file, skipping any line it cannot parse.
    static std::vector<OrderBookEntry> readCSV(std::string csvFilename) {
        std::vector<OrderBookEntry> entries;
        std::ifstream csvFile{csvFilename};
        std::string line;
        int badLines = 0;

        if (!csvFile.is_open()) {
            std::cout << "CSVReader: could not open " << csvFilename << std::endl;
            return entries;
        }

        while (std::getline(csvFile, line)) {
            if (line.empty()) continue;
            try {
                entries.push_back(stringsToOBE(tokenise(line, ',')));
            } catch (const std::exception& e) {
                ++badLines;
            }
        }
        csvFile.close();

        std::cout << "CSVReader: read " << entries.size() << " entries from " << csvFilename;
        if (badLines > 0) std::cout << " (skipped " << badLines << " bad lines)";
        std::cout << std::endl;
        return entries;
    }
};

// ==========================================
// 3. Wallet Class
// ==========================================

class Wallet {
public:
    Wallet() {}

    void insertCurrency(std::string type, double amount) {
        double balance;
        if (amount < 0) throw std::invalid_argument("negative amount");
        if (currencies.count(type) == 0) balance = 0;
        else balance = currencies[type];
        balance += amount;
        currencies[type] = balance;
    }

    bool removeCurrency(std::string type, double amount) {
        if (amount < 0) return false;
        if (currencies.count(type) == 0) return false;
        if (containsCurrency(type, amount)) {
            currencies[type] -= amount;
            return true;
        }
        return false;
    }

    bool containsCurrency(std::string type, double amount) {
        if (currencies.count(type) == 0) return false;
        return currencies[type] >= amount;
    }

    // Can this wallet afford the order it is about to place?
    bool canFulfillOrder(OrderBookEntry order) {
        std::vector<std::string> currs = CSVReader::tokenise(order.product, '/');
        if (currs.size() != 2) return false;

        if (order.orderType == OrderBookType::ask) {
            // To sell ETH for BTC, I need the ETH
            return containsCurrency(currs[0], order.amount);
        }
        if (order.orderType == OrderBookType::bid) {
            // To buy ETH with BTC, I need the BTC
            return containsCurrency(currs[1], order.amount * order.price);
        }
        return false;
    }

    // Move the funds once a sale has actually been matched.
    void processSale(OrderBookEntry& sale) {
        std::vector<std::string> currs = CSVReader::tokenise(sale.product, '/');
        if (currs.size() != 2) return;

        if (sale.orderType == OrderBookType::asksale) {   // we sold
            currencies[currs[0]] -= sale.amount;
            currencies[currs[1]] += sale.amount * sale.price;
        }
        if (sale.orderType == OrderBookType::bidsale) {   // we bought
            currencies[currs[0]] += sale.amount;
            currencies[currs[1]] -= sale.amount * sale.price;
        }
    }

    std::string toString() {
        std::ostringstream s;
        s << std::fixed << std::setprecision(8);
        for (std::pair<std::string, double> pair : currencies) {
            s << "  " << std::setw(6) << std::left << pair.first
              << " : " << pair.second << "\n";
        }
        return s.str();
    }

protected:
    std::map<std::string, double> currencies;
};

// ==========================================
// 4. OrderBook Class
// ==========================================

class OrderBook {
public:
    explicit OrderBook(std::string filename) {
        orders = CSVReader::readCSV(filename);
        if (orders.empty()) {
            std::cout << "OrderBook: falling back to built-in mock data." << std::endl;
            loadMockData();
        }
        std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
    }

    std::vector<std::string> getKnownProducts() {
        std::vector<std::string> products;
        std::map<std::string, bool> prodMap;
        for (OrderBookEntry& e : orders) prodMap[e.product] = true;
        for (auto const& [key, val] : prodMap) products.push_back(key);
        return products;
    }

    std::vector<OrderBookEntry> getOrders(OrderBookType type, std::string product, std::string timestamp) {
        std::vector<OrderBookEntry> orders_sub;
        for (OrderBookEntry& e : orders) {
            if (e.orderType == type && e.product == product && e.timestamp == timestamp) {
                orders_sub.push_back(e);
            }
        }
        return orders_sub;
    }

    static double getHighPrice(std::vector<OrderBookEntry>& orders) {
        if (orders.empty()) throw std::invalid_argument("no orders");
        double max = orders[0].price;
        for (OrderBookEntry& e : orders) if (e.price > max) max = e.price;
        return max;
    }

    static double getLowPrice(std::vector<OrderBookEntry>& orders) {
        if (orders.empty()) throw std::invalid_argument("no orders");
        double min = orders[0].price;
        for (OrderBookEntry& e : orders) if (e.price < min) min = e.price;
        return min;
    }

    std::string getEarliestTime() {
        return orders[0].timestamp;
    }

    std::string getNextTime(std::string timestamp) {
        std::string next_timestamp = "";
        for (OrderBookEntry& e : orders) {
            if (e.timestamp > timestamp) {
                next_timestamp = e.timestamp;
                break;
            }
        }
        if (next_timestamp == "") next_timestamp = orders[0].timestamp; // wrap around
        return next_timestamp;
    }

    void insertOrder(OrderBookEntry& order) {
        orders.push_back(order);
        std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
    }

    std::vector<OrderBookEntry> matchAsksToBids(std::string product, std::string timestamp) {
        std::vector<OrderBookEntry> asks = getOrders(OrderBookType::ask, product, timestamp);
        std::vector<OrderBookEntry> bids = getOrders(OrderBookType::bid, product, timestamp);
        std::vector<OrderBookEntry> sales;

        std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
        std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);

        for (OrderBookEntry& ask : asks) {
            for (OrderBookEntry& bid : bids) {
                if (bid.price >= ask.price) {
                    OrderBookEntry sale{ask.price, 0.0, timestamp, product, OrderBookType::asksale};

                    if (bid.username == "simuser") {
                        sale.username = "simuser";
                        sale.orderType = OrderBookType::bidsale;
                    }
                    if (ask.username == "simuser") {
                        sale.username = "simuser";
                        sale.orderType = OrderBookType::asksale;
                    }

                    if (bid.amount == ask.amount) {
                        sale.amount = ask.amount;
                        sales.push_back(sale);
                        bid.amount = 0;
                        break;
                    }
                    if (bid.amount > ask.amount) {
                        sale.amount = ask.amount;
                        sales.push_back(sale);
                        bid.amount = bid.amount - ask.amount;
                        break;
                    }
                    if (bid.amount < ask.amount && bid.amount > 0) {
                        sale.amount = bid.amount;
                        sales.push_back(sale);
                        ask.amount = ask.amount - bid.amount;
                        bid.amount = 0;
                        continue;
                    }
                }
            }
        }
        return sales;
    }

private:
    void loadMockData() {
        orders.emplace_back(10000, 0.5, "2020/03/17 17:01:24", "BTC/USDT", OrderBookType::bid);
        orders.emplace_back(10500, 0.2, "2020/03/17 17:01:24", "BTC/USDT", OrderBookType::ask);
        orders.emplace_back(10100, 1.0, "2020/03/17 17:01:24", "BTC/USDT", OrderBookType::bid);
        orders.emplace_back(200, 50, "2020/03/17 17:01:30", "ETH/USDT", OrderBookType::ask);
        orders.emplace_back(190, 10, "2020/03/17 17:01:30", "ETH/USDT", OrderBookType::bid);
    }

    std::vector<OrderBookEntry> orders;
};

// ==========================================
// 5. MerkelMain (The App Loop)
// ==========================================

class MerkelMain {
public:
    MerkelMain() {}

    void init() {
        int input;
        currentTime = orderBook.getEarliestTime();
        wallet.insertCurrency("BTC", 10);
        wallet.insertCurrency("ETH", 100);
        wallet.insertCurrency("USDT", 100000);

        while (running) {
            printMenu();
            input = getUserOption();
            processUserOption(input);
        }
        std::cout << "Goodbye." << std::endl;
    }

private:
    void printMenu() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "MERKEL REX TRADING PLATFORM" << std::endl;
        std::cout << "Current Time: " << currentTime << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "1: Print help" << std::endl;
        std::cout << "2: Print exchange stats" << std::endl;
        std::cout << "3: Make an offer (Sell)" << std::endl;
        std::cout << "4: Make a bid (Buy)" << std::endl;
        std::cout << "5: Print wallet" << std::endl;
        std::cout << "6: Continue (Next Time Step)" << std::endl;
        std::cout << "7: Exit" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Type in 1-7: ";
    }

    int getUserOption() {
        int userOption = 0;
        std::string line;
        if (!std::getline(std::cin, line)) {   // end of input (e.g. a piped demo script)
            running = false;
            return 0;
        }
        std::cout << line << std::endl;        // echo, so recorded demos read naturally
        try {
            userOption = std::stoi(line);
        } catch (const std::exception& e) {
            std::cout << "Invalid choice." << std::endl;
        }
        return userOption;
    }

    void processUserOption(int userOption) {
        if (userOption == 0) return;
        if (userOption == 1) printHelp();
        if (userOption == 2) printMarketStats();
        if (userOption == 3) enterAsk();
        if (userOption == 4) enterBid();
        if (userOption == 5) printWallet();
        if (userOption == 6) gotoNextTimeframe();
        if (userOption == 7) running = false;
    }

    void printHelp() {
        std::cout << "Help - Your aim is to make money. Analyse the market and trade." << std::endl;
    }

    void printMarketStats() {
        std::cout << std::fixed << std::setprecision(8);
        for (std::string const& p : orderBook.getKnownProducts()) {
            std::cout << "Product: " << p << std::endl;

            std::vector<OrderBookEntry> asks = orderBook.getOrders(OrderBookType::ask, p, currentTime);
            std::vector<OrderBookEntry> bids = orderBook.getOrders(OrderBookType::bid, p, currentTime);

            if (!asks.empty()) {
                std::cout << "  Asks seen: " << asks.size()
                          << " | max " << OrderBook::getHighPrice(asks)
                          << " | min " << OrderBook::getLowPrice(asks) << std::endl;
            } else {
                std::cout << "  No asks" << std::endl;
            }

            if (!bids.empty()) {
                std::cout << "  Bids seen: " << bids.size()
                          << " | max " << OrderBook::getHighPrice(bids)
                          << " | min " << OrderBook::getLowPrice(bids) << std::endl;
            } else {
                std::cout << "  No bids" << std::endl;
            }

            if (!asks.empty() && !bids.empty()) {
                double spread = OrderBook::getLowPrice(asks) - OrderBook::getHighPrice(bids);
                std::cout << "  Spread   : " << spread << std::endl;
            }
        }
        std::cout << std::defaultfloat;
    }

    void enterAsk() {
        std::cout << "Make an ask - product,price,amount, eg ETH/BTC,0.02,0.5" << std::endl;
        enterOrder(OrderBookType::ask);
    }

    void enterBid() {
        std::cout << "Make a bid - product,price,amount, eg ETH/BTC,0.02,0.5" << std::endl;
        enterOrder(OrderBookType::bid);
    }

    // Both menu options do the same job apart from the order type.
    void enterOrder(OrderBookType type) {
        std::string input;
        if (!std::getline(std::cin, input)) { running = false; return; }
        std::cout << input << std::endl;

        std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
        if (tokens.size() != 3) {
            std::cout << "Bad input! Expected product,price,amount" << std::endl;
            return;
        }
        try {
            OrderBookEntry obe{std::stod(tokens[1]), std::stod(tokens[2]),
                               currentTime, tokens[0], type, "simuser"};
            if (wallet.canFulfillOrder(obe)) {
                std::cout << "Wallet looks good. Order placed." << std::endl;
                orderBook.insertOrder(obe);
            } else {
                std::cout << "Wallet has insufficient funds." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Bad input! Could not read price/amount." << std::endl;
        }
    }

    void printWallet() {
        std::cout << "Wallet:" << std::endl << wallet.toString();
    }

    void gotoNextTimeframe() {
        std::cout << "Going to next time frame..." << std::endl;
        for (std::string& p : orderBook.getKnownProducts()) {
            std::vector<OrderBookEntry> sales = orderBook.matchAsksToBids(p, currentTime);
            std::cout << "Matching " << p << " -> " << sales.size() << " sales" << std::endl;
            for (OrderBookEntry& sale : sales) {
                if (sale.username == "simuser") {
                    std::cout << "  *** YOUR TRADE FILLED *** price " << sale.price
                              << " amount " << sale.amount << std::endl;
                    wallet.processSale(sale);
                }
            }
        }
        currentTime = orderBook.getNextTime(currentTime);
    }

    OrderBook orderBook{"20200317.csv"};
    Wallet wallet;
    std::string currentTime;
    bool running = true;
};

// ==========================================
// Main Entry Point
// ==========================================
// UNIT_TESTS is defined only when tests.cpp includes this file, so that the
// test harness can supply its own main().
#ifndef UNIT_TESTS
int main() {
    MerkelMain app;
    app.init();
    return 0;
}
#endif
