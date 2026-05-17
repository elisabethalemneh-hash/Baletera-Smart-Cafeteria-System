// ============================================================
//   Central Cafeteria — Ordering & Management System
//   Single Cafeteria | C++ Console Application
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstring>
#include "sqlite3.h"
using namespace std;


struct User {
    char studentID[10];   // ETS + 6 digits (9 chars + null)
    char name[26];        // max 25 chars + null
    char phone[11];       // 10 digits + null
    string password;
};

struct Food {
    string name;
    float  price;
    int    preparationTime; // minutes
};

struct Order {
    int    orderNumber;
    string studentID;
    string customerName;
    string customerPhone;
    string foods[10];
    int    quantities[10];
    int    preparationTimes[10];
    int    foodCount;
    float  totalPrice;
    bool   prepared;
    bool   pickedUp;
    int    estimatedTime;
    int    day;
    int    week;
    int    month;
};


int currentDay   = 1;
int currentWeek  = 1;
int currentMonth = 1;
int orderCounter = 0;

const string CAFETERIA_NAME     = "Central Cafeteria";
const string DEFAULT_ADMIN_PASS = "admin123";


sqlite3* db = nullptr;
const string DB_NAME = "Central_cafeteria.db";


string toUpperStr(const string& s) {
    string r = s;
    for (char& c : r) c = (char)toupper((unsigned char)c);
    return r;
}

void pressEnter() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(1000000, '\n');
}

void printLine(char ch = '-', int len = 60) {
    cout << string(len, ch) << "\n";
}

void printHeader(const string& title) {
    cout << "\n";
    printLine('=');
    cout << "  " << CAFETERIA_NAME << "\n";
    cout << "  " << title << "\n";
    printLine('=');
}

int getInt(const string& prompt, int lo, int hi) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        bool ok = true;
        if (s.empty()) ok = false;
        for (char c : s) if (!isdigit((unsigned char)c)) { ok = false; break; }
        if (ok) {
            try {
                long long v = stoll(s);
                if (v >= lo && v <= hi) return (int)v;
            } catch (...) {}
        }
        cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

string getLineInput(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}


bool initializeDatabase() {
    int rc = sqlite3_open(DB_NAME.c_str(), &db);
    if (rc) {
        cerr << " Error opening database: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    cout << " Database '" << DB_NAME << "' opened successfully.\n";

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* createTables = R"(
        CREATE TABLE IF NOT EXISTS Users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            studentID TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            phone TEXT NOT NULL,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS Foods (
            food_id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            price REAL NOT NULL,
            preparation_time INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS Orders (
            order_id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_number INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            total_price REAL NOT NULL,
            estimated_time INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'pending',
            day INTEGER,
            week INTEGER,
            month INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES Users(user_id)
        );

        CREATE TABLE IF NOT EXISTS OrderItems (
            order_item_id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            food_name TEXT NOT NULL,
            quantity INTEGER NOT NULL,
            price_at_time REAL NOT NULL,
            preparation_time INTEGER NOT NULL,
            FOREIGN KEY (order_id) REFERENCES Orders(order_id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS SystemInfo (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            current_day INTEGER DEFAULT 1,
            current_week INTEGER DEFAULT 1,
            current_month INTEGER DEFAULT 1,
            last_order_number INTEGER DEFAULT 0,
            admin_password TEXT DEFAULT 'admin123'
        );
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTables, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        cerr << " Table creation error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }

    sqlite3_exec(db, "ALTER TABLE SystemInfo ADD COLUMN admin_password TEXT DEFAULT 'admin123';",
                 nullptr, nullptr, nullptr);


    sqlite3_exec(db, "INSERT OR IGNORE INTO SystemInfo (id, admin_password) VALUES (1, 'admin123');",
                 nullptr, nullptr, nullptr);

    cout << " Database tables initialized successfully!\n";
    return true;
}

void saveDayInfo() {
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "UPDATE SystemInfo "
                      "SET current_day = ?, "
                      "    current_week = ?, "
                      "    current_month = ? "
                      "WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << " Error preparing saveDayInfo statement." << endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, currentDay);
    sqlite3_bind_int(stmt, 2, currentWeek);
    sqlite3_bind_int(stmt, 3, currentMonth);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << " Warning: Failed to save day info." << endl;
    }
}

void loadDayInfo() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT current_day, current_week, current_month "
                      "FROM SystemInfo WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        currentDay   = sqlite3_column_int(stmt, 0);
        currentWeek  = sqlite3_column_int(stmt, 1);
        currentMonth = sqlite3_column_int(stmt, 2);
    }

    sqlite3_finalize(stmt);
}



void saveCounter() {
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "UPDATE SystemInfo "
                      "SET last_order_number = ? "
                      "WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << " Error preparing saveCounter statement." << endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, orderCounter);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << " Warning: Failed to save order counter." << endl;
    }
}

void loadCounter() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT last_order_number FROM SystemInfo WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        orderCounter = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
}


string loadAdminPass() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT admin_password FROM SystemInfo WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DEFAULT_ADMIN_PASS;
    }

    string pw;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* p = (const char*)sqlite3_column_text(stmt, 0);
        if (p) pw = p;
    }

    sqlite3_finalize(stmt);

    if (!pw.empty())
        return pw;
    else
        return DEFAULT_ADMIN_PASS;
}


void saveAdminPass(const string& pw) {
    if (pw.empty()) return;

    sqlite3_stmt* stmt = nullptr;

    const char* sql = "UPDATE SystemInfo "
                      "SET admin_password = ? "
                      "WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ALTER TABLE SystemInfo ADD COLUMN admin_password TEXT;",
                     nullptr, nullptr, nullptr);

        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    }

    sqlite3_bind_text(stmt, 1, pw.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << " Warning: Failed to save admin password." << endl;
    }
}


vector<User> loadUsers() {
    vector<User> users;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT studentID, name, phone, password FROM Users;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return users;        // Return empty vector if error (same behavior as old code)
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;

        const char* id   = (const char*)sqlite3_column_text(stmt, 0);
        const char* nm   = (const char*)sqlite3_column_text(stmt, 1);
        const char* ph   = (const char*)sqlite3_column_text(stmt, 2);
        const char* pw   = (const char*)sqlite3_column_text(stmt, 3);

        if (id) strncpy(u.studentID, id, 9); u.studentID[9] = '\0';
        if (nm) strncpy(u.name, nm, 25);     u.name[25] = '\0';
        if (ph) strncpy(u.phone, ph, 10);    u.phone[10] = '\0';
        if (pw) u.password = pw;

        users.push_back(u);
    }

    sqlite3_finalize(stmt);
    return users;
}


void saveUsers(const vector<User>& users) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO Users "
                      "(studentID, name, phone, password) "
                      "VALUES (?, ?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << " Error preparing saveUsers statement." << endl;
        return;
    }

    for (const auto& u : users) {
        string studentID_str(u.studentID);
        string name_str(u.name);
        string phone_str(u.phone);

        sqlite3_bind_text(stmt, 1, studentID_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, name_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, phone_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, u.password.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);     // Reset for next user
    }

    sqlite3_finalize(stmt);
}


vector<Food> loadFoods() {
    vector<Food> foods;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name, price, preparation_time FROM Foods;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return foods;        // Return empty if error (same as original)
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Food fd;
        const char* nm = (const char*)sqlite3_column_text(stmt, 0);

        if (nm) fd.name = nm;
        fd.price = sqlite3_column_double(stmt, 1);
        fd.preparationTime = sqlite3_column_int(stmt, 2);

        foods.push_back(fd);
    }

    sqlite3_finalize(stmt);
    return foods;
}

void saveFoods(const vector<Food>& foods) {
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "INSERT OR REPLACE INTO Foods (name, price, preparation_time) "
                      "VALUES (?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << " Error preparing saveFoods statement." << endl;
        return;
    }

    for (const auto& fd : foods) {
        sqlite3_bind_text(stmt, 1, fd.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 1, fd.price);
        sqlite3_bind_int(stmt, 1, fd.preparationTime);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);        // Reset for next food item
    }

    sqlite3_finalize(stmt);
}


string serializeOrder(const Order& o) {
    stringstream ss;
    ss << o.orderNumber   << "|"
       << o.studentID     << "|"
       << o.customerName  << "|"
       << o.customerPhone << "|"
       << o.foodCount     << "|";
    for (int i = 0; i < o.foodCount; i++)
        ss << o.foods[i] << "~" << o.quantities[i] << "~" << o.preparationTimes[i] << ";";
    ss << "|"
       << fixed << setprecision(2) << o.totalPrice << "|"
       << (o.prepared ? 1 : 0) << "|"
       << (o.pickedUp ? 1 : 0) << "|"
       << o.estimatedTime << "|"
       << o.day << "|" << o.week << "|" << o.month;
    return ss.str();
}

Order deserializeOrder(const string& line) {
    Order o;
    o.foodCount = 0; o.totalPrice = 0; o.estimatedTime = 0;
    o.prepared = false; o.pickedUp = false;
    o.day = 1; o.week = 1; o.month = 1;
    for (int i = 0; i < 10; i++) { o.quantities[i] = 0; o.preparationTimes[i] = 0; }
    stringstream ss(line);
    string tok;
    getline(ss, tok, '|'); try { o.orderNumber = stoi(tok); } catch (...) { o.orderNumber = 0; }
    getline(ss, o.studentID,     '|');
    getline(ss, o.customerName,  '|');
    getline(ss, o.customerPhone, '|');
    getline(ss, tok, '|'); try { o.foodCount = stoi(tok); } catch (...) {}
    string foodsBlock;
    getline(ss, foodsBlock, '|');
    {
        stringstream fb(foodsBlock);
        string entry;
        int idx = 0;
        while (getline(fb, entry, ';') && idx < 10) {
            if (entry.empty()) continue;
            stringstream es(entry);
            string nm, qt, pt;
            getline(es, nm, '~');
            getline(es, qt, '~');
            getline(es, pt);
            o.foods[idx] = nm;
            try { o.quantities[idx] = stoi(qt); o.preparationTimes[idx] = stoi(pt); }
            catch (...) {}
            idx++;
        }
    }
    getline(ss, tok, '|'); try { o.totalPrice    = stof(tok); } catch (...) {}
    getline(ss, tok, '|'); o.prepared = (tok == "1");
    getline(ss, tok, '|'); o.pickedUp = (tok == "1");
    getline(ss, tok, '|'); try { o.estimatedTime = stoi(tok); } catch (...) {}
    getline(ss, tok, '|'); try { o.day           = stoi(tok); } catch (...) {}
    getline(ss, tok, '|'); try { o.week          = stoi(tok); } catch (...) {}
    getline(ss, tok);      try { o.month         = stoi(tok); } catch (...) {}
    return o;
}


vector<Order> loadOrders() {
    vector<Order> orders;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        SELECT o.order_number, o.user_id, o.total_price, o.estimated_time,
               o.status, o.day, o.week, o.month,
               u.studentID, u.name, u.phone,
               o.order_id                     -- Important: we need the real order_id
        FROM Orders o
        LEFT JOIN Users u ON o.user_id = u.user_id
        ORDER BY o.order_number DESC;
    )";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Error preparing loadOrders: " << sqlite3_errmsg(db) << endl;
        return orders;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Order o = {};
        o.foodCount = 0;
        o.totalPrice = 0.0f;
        o.estimatedTime = 0;

        o.orderNumber     = sqlite3_column_int(stmt, 0);
        o.totalPrice      = sqlite3_column_double(stmt, 2);
        o.estimatedTime   = sqlite3_column_int(stmt, 3);

        string status = (const char*)sqlite3_column_text(stmt, 4) ?
                       (const char*)sqlite3_column_text(stmt, 4) : "";
        o.prepared = (status == "prepared" || status == "pickedup");
        o.pickedUp = (status == "pickedup");

        o.day   = sqlite3_column_int(stmt, 5);
        o.week  = sqlite3_column_int(stmt, 6);
        o.month = sqlite3_column_int(stmt, 7);

        const char* sid = (const char*)sqlite3_column_text(stmt, 8);
        const char* nm  = (const char*)sqlite3_column_text(stmt, 9);
        const char* ph  = (const char*)sqlite3_column_text(stmt, 10);

        if (sid) o.studentID = sid;
        if (nm)  o.customerName = nm;
        if (ph)  o.customerPhone = ph;

        int order_id = sqlite3_column_int(stmt, 11);   // Real order_id from Orders table

        orders.push_back(o);


        if (order_id > 0) {
            sqlite3_stmt* itemStmt = nullptr;
            const char* itemSql = R"(
                SELECT food_name, quantity, preparation_time
                FROM OrderItems
                WHERE order_id = ?
                ORDER BY order_item_id;
            )";

            sqlite3_prepare_v2(db, itemSql, -1, &itemStmt, nullptr);
            sqlite3_bind_int(itemStmt, 1, order_id);

            int idx = 0;
            while (sqlite3_step(itemStmt) == SQLITE_ROW && idx < 10) {
                const char* foodName = (const char*)sqlite3_column_text(itemStmt, 0);
                if (foodName) {
                    o.foods[idx] = foodName;
                    o.quantities[idx] = sqlite3_column_int(itemStmt, 1);
                    o.preparationTimes[idx] = sqlite3_column_int(itemStmt, 2);
                    o.foodCount++;
                    idx++;
                }
            }
            sqlite3_finalize(itemStmt);

            if (!orders.empty()) {
                orders.back() = o;
            }
        }
    }

    sqlite3_finalize(stmt);
    return orders;
}


void saveOrders(const vector<Order>& orders) {
    if (orders.empty()) return;

    sqlite3_stmt* stmtOrder = nullptr;
    sqlite3_stmt* stmtItem = nullptr;
    sqlite3_stmt* stmtGetOrderId = nullptr;

    // Prepare statements
    const char* orderSql = R"(
        INSERT OR REPLACE INTO Orders
        (order_number, user_id, total_price, estimated_time, status, day, week, month)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    const char* itemSql = R"(
        INSERT INTO OrderItems
        (order_id, food_name, quantity, price_at_time, preparation_time)
        VALUES (?, ?, ?, ?, ?);
    )";

    const char* getOrderIdSql = "SELECT order_id FROM Orders WHERE order_number = ? LIMIT 1;";

    sqlite3_prepare_v2(db, orderSql, -1, &stmtOrder, nullptr);
    sqlite3_prepare_v2(db, itemSql, -1, &stmtItem, nullptr);
    sqlite3_prepare_v2(db, getOrderIdSql, -1, &stmtGetOrderId, nullptr);

    for (const auto& o : orders) {
        // Get user_id
        int userId = 0;
        sqlite3_stmt* tmpStmt = nullptr;
        sqlite3_prepare_v2(db, "SELECT user_id FROM Users WHERE studentID = ? LIMIT 1;",
                          -1, &tmpStmt, nullptr);
        sqlite3_bind_text(tmpStmt, 1, o.studentID.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(tmpStmt) == SQLITE_ROW) {
            userId = sqlite3_column_int(tmpStmt, 0);
        }
        sqlite3_finalize(tmpStmt);

        if (userId == 0) continue;

        string status = o.pickedUp ? "pickedup" : (o.prepared ? "prepared" : "pending");

        sqlite3_bind_int(stmtOrder, 1, o.orderNumber);
        sqlite3_bind_int(stmtOrder, 2, userId);
        sqlite3_bind_double(stmtOrder, 3, o.totalPrice);
        sqlite3_bind_int(stmtOrder, 4, o.estimatedTime);
        sqlite3_bind_text(stmtOrder, 5, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtOrder, 6, o.day);
        sqlite3_bind_int(stmtOrder, 7, o.week);
        sqlite3_bind_int(stmtOrder, 8, o.month);

        sqlite3_step(stmtOrder);
        sqlite3_reset(stmtOrder);

        int realOrderId = 0;
        sqlite3_bind_int(stmtGetOrderId, 1, o.orderNumber);
        if (sqlite3_step(stmtGetOrderId) == SQLITE_ROW) {
            realOrderId = sqlite3_column_int(stmtGetOrderId, 0);
        }
        sqlite3_reset(stmtGetOrderId);

        if (realOrderId == 0) continue;

        sqlite3_stmt* delStmt = nullptr;
        sqlite3_prepare_v2(db, "DELETE FROM OrderItems WHERE order_id = ?;", -1, &delStmt, nullptr);
        sqlite3_bind_int(delStmt, 1, realOrderId);
        sqlite3_step(delStmt);
        sqlite3_finalize(delStmt);

        for (int i = 0; i < o.foodCount; i++) {
            sqlite3_bind_int(stmtItem, 1, realOrderId);                    // Use real order_id
            sqlite3_bind_text(stmtItem, 2, o.foods[i].c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmtItem, 3, o.quantities[i]);
            sqlite3_bind_double(stmtItem, 4, 0.0);                         // price_at_time
            sqlite3_bind_int(stmtItem, 5, o.preparationTimes[i]);

            sqlite3_step(stmtItem);
            sqlite3_reset(stmtItem);
        }
    }


    sqlite3_finalize(stmtOrder);
    sqlite3_finalize(stmtItem);
    sqlite3_finalize(stmtGetOrderId);
}


int getQueueWaitTime() {
    sqlite3_stmt* stmt = nullptr;
    int totalWaitTime = 0;

    const char* sql = R"(
        SELECT SUM(estimated_time)
        FROM Orders
        WHERE day = ?
          AND status IN ('pending', 'prepared');
    )";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, currentDay);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        totalWaitTime = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return totalWaitTime;
}


bool validateStudentID(const string& id) {
    if (id.size() != 9) return false;
    if (toUpperStr(id.substr(0, 3)) != "ETS") return false;
    for (int i = 3; i < 9; i++)
        if (!isdigit((unsigned char)id[i])) return false;
    return true;
}

bool validateName(const string& name) {
    if (name.empty() || name.size() > 25) return false;
    for (char c : name)
        if (!isalpha((unsigned char)c) && c != ' ') return false;
    return true;
}

bool validatePhone(const string& phone) {
    if (phone.size() != 10) return false;
    for (char c : phone)
        if (!isdigit((unsigned char)c)) return false;
    return phone.substr(0, 2) == "07" || phone.substr(0, 2) == "09";
}

bool idExists(const string& id) {
    for (const auto& u : loadUsers())
        if (toUpperStr(string(u.studentID)) == toUpperStr(id)) return true;
    return false;
}

int findUser(const vector<User>& users, const string& id) {
    for (int i = 0; i < (int)users.size(); i++)
        if (toUpperStr(string(users[i].studentID)) == toUpperStr(id)) return i;
    return -1;
}

int findOrderIndex(vector<Order>& orders, int num) {
    for (int i = 0; i < (int)orders.size(); i++)
        if (orders[i].orderNumber == num) return i;
    return -1;
}


void registerUser() {
    printHeader("New User Registration");

    string id;
    while (true) {
        id = getLineInput(" Enter Student ID (e.g. ETS123456): ");
        if (!validateStudentID(id)) {
            cout << " Invalid ID. Must start with ETS followed by exactly 6 digits.\n";
            continue;
        }
        if (idExists(id)) {
            cout << " This Student ID is already registered.\n";
            continue;
        }
        break;
    }

    string name;
    while (true) {
        name = getLineInput(" Enter Full Name (max 25 chars, letters & spaces): ");
        if (!validateName(name)) {
            cout << " Invalid name. Use letters and spaces only, max 25 characters.\n";
            continue;
        }
        break;
    }

    string phone;
    while (true) {
        phone = getLineInput(" Enter Phone Number (10 digits, starts with 07 or 09): ");
        if (!validatePhone(phone)) {
            cout << " Invalid phone. Must be 10 digits starting with 07 or 09.\n";
            continue;
        }
        break;
    }

    cout << " Tip: Use special characters to make your password stronger.\n";
    string pw, pw2;
    while (true) {
        pw = getLineInput(" Create Password: ");
        pw2 = getLineInput(" Confirm Password: ");
        if (pw.empty()) {
            cout << " Password cannot be empty.\n";
            continue;
        }
        if (pw != pw2) {
            cout << " Passwords do not match. Try again.\n";
            continue;
        }
        break;
    }

    string sql = "INSERT INTO Users (studentID, name, phone, password) "
                 "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        cout << " Database error during registration.\n";
        pressEnter();
        return;
    }

    sqlite3_bind_text(stmt, 1, toUpperStr(id).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, pw.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        cout << "\n Registration successful! Welcome, " << name << "!\n";
    } else {
        cout << "\n Registration failed. Please try again.\n";
    }

    pressEnter();
}


int loginUser(vector<User>& users) {
    printHeader("User Login");

    string id = getLineInput("  Enter Student ID: ");
    int idx = findUser(users, id);
    if (idx == -1) {
        cout << "  Student ID not found.\n";
        pressEnter();
        return -1;
    }

    const int MAX_ATTEMPTS = 5;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        string pw = getLineInput("  Enter Password: ");
        if (pw == users[idx].password) {
            cout << "\n  Login successful! Welcome, " << users[idx].name << "!\n";
            pressEnter();
            return idx;
        }
        int left = MAX_ATTEMPTS - attempt;
        if (left > 0)
            cout << "  Wrong password. You have " << left << " attempt(s) left.\n";
    }
    cout << "  Your attempt has ended. Try again later.\n";
    pressEnter();
    return -1;
}


bool viewCafeteriaMenu() {
    printHeader("Cafeteria Menu");
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << "  No foods available at the moment.\n";
        pressEnter();
        return false;
    }
    cout << "  " << left << setw(4) << "#"
         << setw(26) << "Food"
         << setw(14) << "Price"
         << "Prep Time\n";
    printLine();
    for (int i = 0; i < (int)foods.size(); i++) {
        cout << "  " << setw(3) << (i+1) << ". "
             << setw(23) << foods[i].name
             << setw(14) << (to_string((int)foods[i].price) + " birr")
             << foods[i].preparationTime << " min\n";
    }
    printLine();
    cout << "\n  Do you want to order now?\n";
    cout << "  1. Yes\n  2. No\n";
    int ch = getInt("  Choice: ", 1, 2);
    return (ch == 1);
}




void orderFood(const User& user) {
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << " No foods available to order.\n";
        pressEnter();
        return;
    }

    printHeader("Order Food");
    cout << " " << left << setw(4) << "#"
         << setw(26) << "Food"
         << setw(14) << "Price"
         << "Prep Time\n";
    printLine();

    for (int i = 0; i < (int)foods.size(); i++) {
        cout << " " << setw(3) << (i+1) << ". "
             << setw(23) << foods[i].name
             << setw(14) << (to_string((int)foods[i].price) + " birr")
             << foods[i].preparationTime << " min\n";
    }
    printLine();

    int numItems = getInt(" How many different food items? (1-10): ", 1, 10);

    Order o;
    o.foodCount = 0;
    o.totalPrice = 0.0f;
    o.studentID = string(user.studentID);
    o.customerName = string(user.name);
    o.customerPhone = string(user.phone);
    o.prepared = false;
    o.pickedUp = false;
    o.day = currentDay;
    o.week = currentWeek;
    o.month = currentMonth;

    int ownPrepTime = 0;
    for (int i = 0; i < numItems; i++) {
        cout << "\n Item " << (i+1) << ":\n";
        int choice = getInt(" Select food number: ", 1, (int)foods.size());
        int qty = getInt(" Enter quantity: ", 1, 50);

        o.foods[o.foodCount] = foods[choice-1].name;
        o.quantities[o.foodCount] = qty;
        o.preparationTimes[o.foodCount] = foods[choice-1].preparationTime;
        o.totalPrice += foods[choice-1].price * qty;

        if (foods[choice-1].preparationTime > ownPrepTime)
            ownPrepTime = foods[choice-1].preparationTime;

        o.foodCount++;
    }

    int queueWait = getQueueWaitTime();
    o.estimatedTime = queueWait + ownPrepTime;

    printLine();
    cout << " ORDER SUMMARY\n";
    printLine();
    for (int i = 0; i < o.foodCount; i++)
        cout << " - " << o.foods[i] << " x" << o.quantities[i]
             << " (" << o.preparationTimes[i] << " min)\n";

    cout << " Total Price     : " << fixed << setprecision(2) << o.totalPrice << " birr\n";
    cout << " Queue Wait      : " << queueWait << " min\n";
    cout << " Estimated Pickup: " << o.estimatedTime << " minutes\n";
    printLine();

    string pw = getLineInput(" Confirm your password to place order: ");
    if (pw != string(user.password)) {
        cout << " Wrong password. Order cancelled.\n";
        pressEnter();
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    int newOrderNum = 0;
    int userId = 0;
    int realOrderId = 0;

    const char* userSql = "SELECT user_id FROM Users WHERE studentID = ?;";
    sqlite3_prepare_v2(db, userSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, o.studentID.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (userId == 0) {
        cout << " Error: User not found in database.\n";
        pressEnter();
        return;
    }

    sqlite3_prepare_v2(db, "SELECT last_order_number FROM SystemInfo WHERE id=1", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        newOrderNum = sqlite3_column_int(stmt, 0) + 1;
    }
    sqlite3_finalize(stmt);


    const char* orderSql = "INSERT INTO Orders "
                           "(order_number, user_id, total_price, estimated_time, status, day, week, month) "
                           "VALUES (?, ?, ?, ?, 'pending', ?, ?, ?);";

    sqlite3_prepare_v2(db, orderSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, newOrderNum);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_double(stmt, 3, o.totalPrice);
    sqlite3_bind_int(stmt, 4, o.estimatedTime);
    sqlite3_bind_int(stmt, 5, o.day);
    sqlite3_bind_int(stmt, 6, o.week);
    sqlite3_bind_int(stmt, 7, o.month);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cout << " Failed to place order. Database error.\n";
        pressEnter();
        return;
    }

    const char* getIdSql = "SELECT order_id FROM Orders WHERE order_number = ? LIMIT 1;";
    sqlite3_prepare_v2(db, getIdSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, newOrderNum);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        realOrderId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (realOrderId == 0) {
        cout << " Failed to retrieve order ID.\n";
        pressEnter();
        return;
    }

    const char* itemSql = "INSERT INTO OrderItems "
                          "(order_id, food_name, quantity, price_at_time, preparation_time) "
                          "VALUES (?, ?, ?, ?, ?);";

    sqlite3_prepare_v2(db, itemSql, -1, &stmt, nullptr);

    for (int i = 0; i < o.foodCount; i++) {
        sqlite3_bind_int(stmt, 1, realOrderId);                    // ← Fixed: Using real order_id
        sqlite3_bind_text(stmt, 2, o.foods[i].c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, o.quantities[i]);
        sqlite3_bind_double(stmt, 4, foods[i].price);
        sqlite3_bind_int(stmt, 5, o.preparationTimes[i]);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    sqlite3_exec(db, "UPDATE SystemInfo SET last_order_number = last_order_number + 1 WHERE id=1",
                 nullptr, nullptr, nullptr);

    cout << "\n Order placed successfully!\n";
    cout << " Order Number : #" << newOrderNum << "\n";
    cout << " Estimated Pickup: " << o.estimatedTime << " minutes\n";

    pressEnter();
}


void viewCurrentOrderStatus(const string& studentID) {
    printHeader("Current Order Status");
    vector<Order> orders = loadOrders();
    bool found = false;
    for (const auto& o : orders) {
        if (toUpperStr(o.studentID) != toUpperStr(studentID)) continue;
        if (o.pickedUp) continue;
        found = true;
        printLine();
        cout << "  Order #" << o.orderNumber
             << "  |  Day " << o.day << "  Week " << o.week << "  Month " << o.month << "\n";
        cout << "  Foods:\n";
        for (int i = 0; i < o.foodCount; i++)
            cout << "    - " << o.foods[i] << " x" << o.quantities[i] << "\n";
        cout << "  Total Price     : " << fixed << setprecision(2) << o.totalPrice << " birr\n";
        cout << "  Estimated Pickup: " << o.estimatedTime << " minutes\n";
        string status = o.pickedUp ? "Picked Up" : (o.prepared ? "Prepared" : "Waiting");
        cout << "  Status          : " << status << "\n";
    }
    if (!found) cout << "  No active orders found.\n";
    pressEnter();
}

void viewOrderHistory(const string& studentID) {
    printHeader("Order History");
    vector<Order> orders = loadOrders();
    bool found = false;
    for (const auto& o : orders) {
        if (toUpperStr(o.studentID) != toUpperStr(studentID)) continue;
        found = true;
        printLine();
        cout << "  Order #" << o.orderNumber
             << "  |  Day " << o.day << "  Week " << o.week << "  Month " << o.month << "\n";
        cout << "  Foods:\n";
        for (int i = 0; i < o.foodCount; i++)
            cout << "    - " << o.foods[i] << " x" << o.quantities[i] << "\n";
        cout << "  Total Spent     : " << fixed << setprecision(2) << o.totalPrice << " birr\n";
        string status = o.pickedUp ? "Picked Up" : (o.prepared ? "Prepared (not picked)" : "Waiting");
        cout << "  Status          : " << status << "\n";
    }
    if (!found) cout << "  No order history found.\n";
    pressEnter();
}

void viewOrdersMenu(const string& studentID) {
    while (true) {
        printHeader("My Orders");
        cout << "  1. Current Order Status\n";
        cout << "  2. Order History\n";
        cout << "  3. Back\n";
        int ch = getInt("  Choice: ", 1, 3);
        if      (ch == 1) viewCurrentOrderStatus(studentID);
        else if (ch == 2) viewOrderHistory(studentID);
        else return;
    }
}


void userChangePassword(User& user, vector<User>& users, int idx) {
    printHeader("Change Password");
    string oldPw = getLineInput("  Enter current password: ");
    if (oldPw != user.password) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }
    string newPw, confirm;
    while (true) {
        newPw   = getLineInput("  Enter new password: ");
        confirm = getLineInput("  Confirm new password: ");
        if (newPw.empty())    { cout << "  Password cannot be empty.\n"; continue; }
        if (newPw != confirm) { cout << "  Passwords do not match.\n";   continue; }
        break;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE Users SET password = ? WHERE studentID = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "  Database error. Please try again.\n";
        pressEnter();
        return;
    }

    sqlite3_bind_text(stmt, 1, newPw.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.studentID, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {

        user.password       = newPw;
        users[idx].password = newPw;
        cout << "  Password changed successfully.\n";
    } else {
        cout << "  Failed to update password. Please try again.\n";
    }

    pressEnter();
}


void userMenu(int userIdx) {
    vector<User> users = loadUsers();
    if (userIdx < 0 || userIdx >= (int)users.size()) return;
    User& user = users[userIdx];

    while (true) {
        printHeader("User Menu — " + string(user.name));
        cout << "  1. View Cafeteria Menu\n";
        cout << "  2. Order Food\n";
        cout << "  3. View Orders\n";
        cout << "  4. Change Password\n";
        cout << "  5. Logout\n";
        int ch = getInt("  Choice: ", 1, 5);

        if (ch == 1) {
            bool wantOrder = viewCafeteriaMenu();
            if (wantOrder) orderFood(user);
        } else if (ch == 2) {
            orderFood(user);
        } else if (ch == 3) {
            viewOrdersMenu(string(user.studentID));
        } else if (ch == 4) {
            userChangePassword(user, users, userIdx);
        } else {
            cout << "  Logged out. Goodbye, " << user.name << "!\n";
            pressEnter();
            return;
        }
    }
}


void adminInsertFood() {
    printHeader("Insert New Food");

    string name = getLineInput(" Food Name: ");
    if (name.empty()) {
        cout << " Name cannot be empty.\n";
        pressEnter();
        return;
    }

    float price = 0;
    while (true) {
        string s = getLineInput(" Price (birr): ");
        try {
            price = stof(s);
            if (price > 0) break;
        } catch (...) {}
        cout << " Enter a valid positive price.\n";
    }

    int prepTime = getInt(" Preparation Time (minutes): ", 1, 300);

    sqlite3_stmt* stmt = nullptr;
    string checkSql = "SELECT food_id FROM Foods WHERE name = ? COLLATE NOCASE;";
    sqlite3_prepare_v2(db, checkSql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cout << " A food with that name already exists.\n";
        sqlite3_finalize(stmt);
        pressEnter();
        return;
    }
    sqlite3_finalize(stmt);


    string insertSql = "INSERT INTO Foods (name, price, preparation_time) "
                       "VALUES (?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, insertSql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << " Database error.\n";
        pressEnter();
        return;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, price);
    sqlite3_bind_int(stmt, 3, prepTime);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        cout << " Food added successfully.\n";
    } else {
        cout << " Failed to add food.\n";
    }

    pressEnter();
}


void adminUpdateFoodPrice() {
    printHeader("Update Food Price");
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << " No foods available.\n";
        pressEnter();
        return;
    }

    string name = getLineInput(" Enter food name to update: ");
    bool found = false;

    for (auto& f : foods) {
        if (toUpperStr(f.name) == toUpperStr(name)) {
            float price = 0;
            while (true) {
                string s = getLineInput(" New Price (birr): ");
                try {
                    price = stof(s);
                    if (price > 0) break;
                } catch (...) {}
                cout << " Enter a valid positive price.\n";
            }


            sqlite3_stmt* stmt = nullptr;
            const char* delSql = "DELETE FROM Foods WHERE UPPER(name) = UPPER(?);";
            sqlite3_prepare_v2(db, delSql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);


            const char* insertSql = "INSERT INTO Foods (name, price, preparation_time) "
                                    "VALUES (?, ?, ?);";

            sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, f.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 2, price);
            sqlite3_bind_int(stmt, 3, f.preparationTime);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            f.price = price;
            saveFoods(foods);   // Keep consistency with existing pattern

            cout << " Price updated successfully.\n";
            found = true;
            break;
        }
    }

    if (!found)
        cout << " Food not found.\n";

    pressEnter();
}
void adminUpdatePrepTime() {
    printHeader("Update Preparation Time");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
    string name = getLineInput("  Enter food name to update: ");
    bool found = false;
    for (auto& f : foods) {
        if (toUpperStr(f.name) == toUpperStr(name)) {
            int pt = getInt("  New Preparation Time (minutes): ", 1, 300);
            f.preparationTime = pt;
            saveFoods(foods);
            cout << "  Preparation time updated successfully.\n";
            found = true;
            break;
        }
    }
    if (!found) cout << "  Food not found.\n";
    pressEnter();
}


void adminDeleteFood() {
    printHeader("Delete Food");
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << " No foods available.\n";
        pressEnter();
        return;
    }

    string name = getLineInput(" Enter food name to delete: ");

    int before = (int)foods.size();

    foods.erase(remove_if(foods.begin(), foods.end(),
        [&](const Food& f){
            return toUpperStr(f.name) == toUpperStr(name);
        }),
        foods.end());

    if ((int)foods.size() < before) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "DELETE FROM Foods WHERE UPPER(name) = UPPER(?);";

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        saveFoods(foods);
        cout << " Food deleted successfully.\n";
    } else {
        cout << " Food not found.\n";
    }
    pressEnter();
}

void adminViewFoods() {
    printHeader("Food List");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
    cout << "  " << left << setw(4) << "#"
         << setw(26) << "Food"
         << setw(14) << "Price"
         << "Prep Time\n";
    printLine();
    for (int i = 0; i < (int)foods.size(); i++) {
        cout << "  " << setw(3) << (i+1) << ". "
             << setw(23) << foods[i].name
             << setw(14) << (to_string((int)foods[i].price) + " birr")
             << foods[i].preparationTime << " min\n";
    }
    pressEnter();
}

void adminEditMenu() {
    while (true) {
        printHeader("Edit Menu");
        cout << "  1. Insert New Food\n";
        cout << "  2. Update Food Price\n";
        cout << "  3. Update Preparation Time\n";
        cout << "  4. Delete Food\n";
        cout << "  5. View Foods\n";
        cout << "  6. Back\n";
        int ch = getInt("  Choice: ", 1, 6);
        if      (ch == 1) adminInsertFood();
        else if (ch == 2) adminUpdateFoodPrice();
        else if (ch == 3) adminUpdatePrepTime();
        else if (ch == 4) adminDeleteFood();
        else if (ch == 5) adminViewFoods();
        else return;
    }
}



void adminViewWaitingOrders() {
    printHeader("Waiting Orders");
    vector<Order> orders = loadOrders();
    bool found = false;
    for (const auto& o : orders) {
        if (o.prepared) continue;
        found = true;
        printLine();
        cout << "  Order #" << o.orderNumber
             << "  |  Day " << o.day << "  Week " << o.week << "  Month " << o.month << "\n";
        cout << "  Customer : " << o.customerName << "\n";
        cout << "  Phone    : " << o.customerPhone << "\n";
        cout << "  Foods:\n";
        for (int i = 0; i < o.foodCount; i++)
            cout << "    - " << o.foods[i] << " x" << o.quantities[i] << "\n";
        cout << "  Estimated Pickup: " << o.estimatedTime << " minutes\n";
        cout << "  Status   : Waiting\n";
    }
    if (!found) cout << "  No waiting orders.\n";
    pressEnter();
}

void adminMarkPrepared() {
    printHeader("Mark Order as Prepared");
    int num = getInt("  Enter Order Number: ", 1, 999999);
    vector<Order> orders = loadOrders();
    int idx = findOrderIndex(orders, num);
    if (idx == -1) { cout << "  Order not found.\n"; pressEnter(); return; }
    if (orders[idx].prepared) { cout << "  Order is already marked as prepared.\n"; pressEnter(); return; }
    orders[idx].prepared = true;
    saveOrders(orders);
    cout << "  Order #" << num << " marked as Prepared.\n";
    pressEnter();
}

void adminViewPreparedUnpicked() {
    printHeader("Prepared but Unpicked Orders");
    vector<Order> orders = loadOrders();
    bool found = false;
    for (const auto& o : orders) {
        if (!o.prepared || o.pickedUp) continue;
        found = true;
        printLine();
        cout << "  Order #" << o.orderNumber
             << "  |  Day " << o.day << "  Week " << o.week << "  Month " << o.month << "\n";
        cout << "  Customer : " << o.customerName << "\n";
        cout << "  Phone    : " << o.customerPhone << "\n";
        cout << "  Foods:\n";
        for (int i = 0; i < o.foodCount; i++)
            cout << "    - " << o.foods[i] << " x" << o.quantities[i] << "\n";
        cout << "  Estimated Pickup: " << o.estimatedTime << " minutes\n";
        cout << "  Status   : Prepared — Awaiting Pickup\n";
    }
    if (!found) cout << "  No prepared-but-unpicked orders.\n";
    pressEnter();
}


void adminMarkPickedUp() {
    printHeader("Mark Order as Picked Up");
    int num = getInt("  Enter Order Number: ", 1, 999999);


    sqlite3_stmt* stmt = nullptr;
    const char* checkSql = R"(
        SELECT status
        FROM Orders
        WHERE order_number = ?;
    )";

    int rc = sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "  Database error.\n";
        pressEnter();
        return;
    }

    sqlite3_bind_int(stmt, 1, num);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        cout << "  Order not found.\n";
        sqlite3_finalize(stmt);
        pressEnter();
        return;
    }

    string currentStatus = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);

    if (currentStatus == "pickedup") {
        cout << "  Order is already marked as picked up.\n";
        pressEnter();
        return;
    }
    if (currentStatus != "prepared") {
        cout << "  Order has not been prepared yet.\n";
        pressEnter();
        return;
    }

    const char* updateSql = "UPDATE Orders SET status = 'pickedup' WHERE order_number = ?;";

    rc = sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "  Database error.\n";
        pressEnter();
        return;
    }

    sqlite3_bind_int(stmt, 1, num);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        cout << "  Order #" << num << " marked as Picked Up.\n";
    } else {
        cout << "  Failed to update order status.\n";
    }

    pressEnter();
}


void adminViewCustomers() {
    printHeader("Customer Information");
    vector<User> users = loadUsers();
    if (users.empty()) { cout << "  No registered users.\n"; pressEnter(); return; }
    cout << "  " << left << setw(12) << "Student ID"
         << setw(28) << "Name"
         << "Phone\n";
    printLine();
    for (const auto& u : users)
        cout << "  " << setw(12) << u.studentID
             << setw(28) << u.name
             << u.phone << "\n";
    pressEnter();
}


void generateReport(const string& label, int filterDay, int filterWeek, int filterMonth) {
    // filterDay/Week/Month == -1 means "all"

    sqlite3_stmt* stmt = nullptr;
    float totalRevenue = 0;
    int totalOrders = 0, pickedUp = 0, unpicked = 0;

    string sql = R"(
        SELECT o.total_price, o.status,
               oi.food_name, oi.quantity
        FROM Orders o
        LEFT JOIN OrderItems oi ON o.order_number = oi.order_id
        WHERE 1=1
    )";

    if (filterDay   != -1) sql += " AND o.day = ?";
    if (filterWeek  != -1) sql += " AND o.week = ?";
    if (filterMonth != -1) sql += " AND o.month = ?";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "  Database error generating report.\n";
        pressEnter();
        return;
    }

    int param = 1;
    if (filterDay   != -1) sqlite3_bind_int(stmt, param++, filterDay);
    if (filterWeek  != -1) sqlite3_bind_int(stmt, param++, filterWeek);
    if (filterMonth != -1) sqlite3_bind_int(stmt, param++, filterMonth);

    vector<string> foodNames;
    vector<int>    foodSales;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        float price = sqlite3_column_double(stmt, 0);
        string status = (const char*)sqlite3_column_text(stmt, 1);
        const char* foodName = (const char*)sqlite3_column_text(stmt, 2);
        int qty = sqlite3_column_int(stmt, 3);

        totalRevenue += price;
        totalOrders++;

        if (status == "pickedup") pickedUp++;
        else unpicked++;

        if (foodName && qty > 0) {
            bool found = false;
            for (int j = 0; j < (int)foodNames.size(); j++) {
                if (toUpperStr(foodNames[j]) == toUpperStr(foodName)) {
                    foodSales[j] += qty;
                    found = true;
                    break;
                }
            }
            if (!found) {
                foodNames.push_back(foodName);
                foodSales.push_back(qty);
            }
        }
    }

    sqlite3_finalize(stmt);

    printHeader(label + " Report");
    cout << " Total Orders    : " << totalOrders << "\n";
    cout << " Total Revenue   : " << fixed << setprecision(2) << totalRevenue << " birr\n";
    cout << " Picked Up       : " << pickedUp << "\n";
    cout << " Unpicked        : " << unpicked << "\n";

    if (!foodNames.empty()) {
        cout << "\n Foods Sold:\n";
        int maxSales = 0;
        string mostOrdered;
        for (int i = 0; i < (int)foodNames.size(); i++) {
            cout << " - " << foodNames[i] << ": " << foodSales[i] << " units\n";
            if (foodSales[i] > maxSales) {
                maxSales = foodSales[i];
                mostOrdered = foodNames[i];
            }
        }
        if (!mostOrdered.empty())
            cout << "\n Most Ordered    : " << mostOrdered << " (" << maxSales << " units)\n";
    }
    pressEnter();
}
void adminReports() {
    while (true) {
        printHeader("Reports");
        cout << "  1. Daily Report  (Day " << currentDay << ")\n";
        cout << "  2. Weekly Report (Week " << currentWeek << ")\n";
        cout << "  3. Monthly Report (Month " << currentMonth << ")\n";
        cout << "  4. Back\n";
        int ch = getInt("  Choice: ", 1, 4);
        if      (ch == 1) generateReport("Daily",   currentDay,   -1,          -1);
        else if (ch == 2) generateReport("Weekly",  -1,           currentWeek, -1);
        else if (ch == 3) generateReport("Monthly", -1,           -1,          currentMonth);
        else return;
    }
}


void adminChangeDay() {
    printHeader("Change Day");
    cout << "  Current: Day " << currentDay
         << "  Week " << currentWeek
         << "  Month " << currentMonth << "\n\n";
    cout << "  Do you want to advance to the next day?\n";
    cout << "  1. Yes\n  2. No\n";
    int ch = getInt("  Choice: ", 1, 2);
    if (ch == 2) return;

    currentDay++;
    if (currentDay % 7 == 1 && currentDay > 1) currentWeek++;
    if (currentDay % 30 == 1 && currentDay > 1) currentMonth++;

    // Reset order counter for new day
    orderCounter = 0;


    saveDayInfo();
    saveCounter();

    cout << "  Day advanced. Now: Day " << currentDay
         << "  Week " << currentWeek
         << "  Month " << currentMonth << "\n";
    pressEnter();
}


void adminChangePassword() {
    printHeader("Admin Change Password");

    string adminPw = loadAdminPass();
    string oldPw = getLineInput("  Enter current admin password: ");
    if (oldPw != adminPw) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }

    string newPw, confirm;
    while (true) {
        newPw   = getLineInput("  Enter new password: ");
        confirm = getLineInput("  Confirm new password: ");
        if (newPw.empty())    { cout << "  Password cannot be empty.\n"; continue; }
        if (newPw != confirm) { cout << "  Passwords do not match.\n";   continue; }
        break;
    }


    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE SystemInfo SET admin_password = ? WHERE id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {

        sqlite3_exec(db, "ALTER TABLE SystemInfo ADD COLUMN admin_password TEXT;",
                     nullptr, nullptr, nullptr);
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    }

    sqlite3_bind_text(stmt, 1, newPw.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        cout << "  Admin password changed successfully.\n";
    } else {
        cout << "  Failed to change admin password.\n";
    }

    pressEnter();
}


void adminMenu() {
    while (true) {
        printHeader("Admin Menu");
        cout << "  Day " << currentDay
             << "  |  Week " << currentWeek
             << "  |  Month " << currentMonth << "\n\n";
        cout << "  1.  Edit Menu\n";
        cout << "  2.  View Waiting Orders\n";
        cout << "  3.  View Prepared but Unpicked Orders\n";
        cout << "  4.  Mark Order as Prepared\n";
        cout << "  5.  Mark Order as Picked Up\n";
        cout << "  6.  View Customer Information\n";
        cout << "  7.  Reports\n";
        cout << "  8.  Change Day\n";
        cout << "  9.  Change Password\n";
        cout << "  10. Logout\n";
        int ch = getInt("  Choice: ", 1, 10);
        if      (ch == 1)  adminEditMenu();
        else if (ch == 2)  adminViewWaitingOrders();
        else if (ch == 3)  adminViewPreparedUnpicked();
        else if (ch == 4)  adminMarkPrepared();
        else if (ch == 5)  adminMarkPickedUp();
        else if (ch == 6)  adminViewCustomers();
        else if (ch == 7)  adminReports();
        else if (ch == 8)  adminChangeDay();
        else if (ch == 9)  adminChangePassword();
        else {
            cout << "  Admin logged out.\n";
            pressEnter();
            return;
        }
    }
}

void adminLogin() {
    printHeader("Admin Login");
    string adminPw = loadAdminPass();
    string entered = getLineInput("  Enter Admin Password: ");
    if (entered != adminPw) {
        cout << "  Incorrect admin password.\n";
        pressEnter();
        return;
    }
    cout << "  Access granted.\n";
    pressEnter();
    adminMenu();
}


void userPartMenu() {
    while (true) {
        printHeader("User Section");
        cout << "  1. New User Registration\n";
        cout << "  2. Previous User Login\n";
        cout << "  3. Back\n";
        int ch = getInt("  Choice: ", 1, 3);
        if (ch == 1) {
            registerUser();
        } else if (ch == 2) {
            vector<User> users = loadUsers();
            int idx = loginUser(users);
            if (idx != -1) userMenu(idx);
        } else {
            return;
        }
    }
}

int main() {
    initializeDatabase();
    loadDayInfo();
    loadCounter();

    while (true) {
        printHeader("Main Menu");
        cout << "  1. User\n";
        cout << "  2. Admin\n";
        cout << "  3. Exit\n";
        int ch = getInt("  Choice: ", 1, 3);
        if      (ch == 1) userPartMenu();
        else if (ch == 2) adminLogin();
        else {
            cout << "\n  Thank you for using " << CAFETERIA_NAME << ". Goodbye!\n\n";
            return 0;
        }
    }
}
