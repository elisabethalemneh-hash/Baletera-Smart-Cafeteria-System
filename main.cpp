#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstring>
#include "sqlite3.h"
using namespace std;

// ============================================================
// STRUCTURES
// ============================================================
struct User {
    char studentID[10]{};
    char name[26]{};
    char phone[11]{};
    string password;
};

struct Food {
    string name;
    float price = 0.0f;
    int preparationTime = 0;
};

struct Order {
    int orderNumber = 0;
    string studentID, customerName, customerPhone;
    string foods[10];
    int quantities[10]{}, preparationTimes[10]{};
    int foodCount = 0;
    float totalPrice = 0.0f;
    bool prepared = false, pickedUp = false;
    int estimatedTime = 0, day = 1, week = 1, month = 1;
};

// ============================================================
// GLOBALS
// ============================================================
int currentDay = 1, currentWeek = 1, currentMonth = 1;
sqlite3* db = nullptr;
const string DB_NAME = "central_cafeteria.db";
const string DEFAULT_ADMIN_PASS = "admin123";

// ============================================================
// UTILITIES
// ============================================================
string toUpperStr(string s) {
    for (char& c : s) c = toupper((unsigned char)c);
    return s;
}

void pressEnter() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(1000000, '\n');
}

void printLine(int len) { cout << string(len, '-') << '\n'; }

void printHeader(const string& title) {
    cout << "\n"; printLine();
    cout << "  Central Cafeteria\n  " << title << "\n";
    printLine();
}

int getInt(const string& prompt, int lo, int hi) {
    while (true) {
        cout << prompt;
        string s; getline(cin, s);
        try {
            long long v = stoll(s);
            if (v >= lo && v <= hi) return (int)v;
        } catch (...) {}
        cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

string getLineInput(const string& prompt) {
    cout << prompt;
    string s; getline(cin, s);
    return s;
}

// ============================================================
// DATABASE INITIALIZATION
// ============================================================
bool initializeDatabase() {
    if (sqlite3_open(DB_NAME.c_str(), &db)) {
        cerr << " Error opening database.\n";
        return false;
    }
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
            status TEXT DEFAULT 'pending',
            day INTEGER, week INTEGER, month INTEGER,
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
    sqlite3_exec(db, createTables, nullptr, nullptr, nullptr);
    sqlite3_exec(db, "INSERT OR IGNORE INTO SystemInfo (id, admin_password) VALUES (1, 'admin123');", nullptr, nullptr, nullptr);
    cout << " Database initialized successfully!\n";
    return true;
}

// ============================================================
// CORE DB FUNCTIONS
// ============================================================
vector<User> loadUsers() {
    vector<User> users;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT studentID, name, phone, password FROM Users";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            User u;
            strcpy(u.studentID, (const char*)sqlite3_column_text(stmt, 0));
            strcpy(u.name, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(u.phone, (const char*)sqlite3_column_text(stmt, 2));
            u.password = (const char*)sqlite3_column_text(stmt, 3);
            users.push_back(u);
        }
    }
    sqlite3_finalize(stmt);
    return users;
}

vector<Food> loadFoods() {
    vector<Food> foods;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT name, price, preparation_time FROM Foods";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Food f;
            f.name = (const char*)sqlite3_column_text(stmt, 0);
            f.price = (float)sqlite3_column_double(stmt, 1);
            f.preparationTime = sqlite3_column_int(stmt, 2);
            foods.push_back(f);
        }
    }
    sqlite3_finalize(stmt);
    return foods;
}

vector<Order> loadOrders() {
    vector<Order> orders;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT order_number, studentID, name, phone, total_price, estimated_time, status, day, week, month FROM Orders JOIN Users ON Orders.user_id = Users.user_id";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Order o;
            o.orderNumber = sqlite3_column_int(stmt, 0);
            o.studentID = (const char*)sqlite3_column_text(stmt, 1);
            o.customerName = (const char*)sqlite3_column_text(stmt, 2);
            o.customerPhone = (const char*)sqlite3_column_text(stmt, 3);
            o.totalPrice = (float)sqlite3_column_double(stmt, 4);
            o.estimatedTime = sqlite3_column_int(stmt, 5);
            string status = (const char*)sqlite3_column_text(stmt, 6);
            o.prepared = (status == "prepared");
            o.pickedUp = (status == "picked_up");
            o.day = sqlite3_column_int(stmt, 7);
            o.week = sqlite3_column_int(stmt, 8);
            o.month = sqlite3_column_int(stmt, 9);
            // load order items
            sqlite3_stmt* stmt2;
            const char* sql2 = "SELECT food_name, quantity, preparation_time FROM OrderItems WHERE order_id = (SELECT order_id FROM Orders WHERE order_number = ?)";
            if (sqlite3_prepare_v2(db, sql2, -1, &stmt2, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt2, 1, o.orderNumber);
                o.foodCount = 0;
                while (sqlite3_step(stmt2) == SQLITE_ROW && o.foodCount < 10) {
                    o.foods[o.foodCount] = (const char*)sqlite3_column_text(stmt2, 0);
                    o.quantities[o.foodCount] = sqlite3_column_int(stmt2, 1);
                    o.preparationTimes[o.foodCount] = sqlite3_column_int(stmt2, 2);
                    o.foodCount++;
                }
            }
            sqlite3_finalize(stmt2);
            orders.push_back(o);
        }
    }
    sqlite3_finalize(stmt);
    return orders;
}

void saveUsers(const vector<User>& users) {
    sqlite3_exec(db, "DELETE FROM Users", nullptr, nullptr, nullptr);
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Users (studentID, name, phone, password) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    for (const auto& u : users) {
        sqlite3_bind_text(stmt, 1, u.studentID, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, u.name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, u.phone, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, u.password.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}

void saveFoods(const vector<Food>& foods) {
    sqlite3_exec(db, "DELETE FROM Foods", nullptr, nullptr, nullptr);
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Foods (name, price, preparation_time) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    for (const auto& f : foods) {
        sqlite3_bind_text(stmt, 1, f.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, f.price);
        sqlite3_bind_int(stmt, 3, f.preparationTime);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}

void saveOrders(const vector<Order>& orders) {
    sqlite3_exec(db, "DELETE FROM Orders", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "DELETE FROM OrderItems", nullptr, nullptr, nullptr);
    for (const auto& o : orders) {
        // find user_id
        int user_id = -1;
        sqlite3_stmt* stmt_user;
        const char* sql_user = "SELECT user_id FROM Users WHERE studentID = ?";
        sqlite3_prepare_v2(db, sql_user, -1, &stmt_user, nullptr);
        sqlite3_bind_text(stmt_user, 1, o.studentID.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_user) == SQLITE_ROW)
            user_id = sqlite3_column_int(stmt_user, 0);
        sqlite3_finalize(stmt_user);
        if (user_id == -1) continue;

        string status = "pending";
        if (o.pickedUp) status = "picked_up";
        else if (o.prepared) status = "prepared";

        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO Orders (order_number, user_id, total_price, estimated_time, status, day, week, month) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, o.orderNumber);
        sqlite3_bind_int(stmt, 2, user_id);
        sqlite3_bind_double(stmt, 3, o.totalPrice);
        sqlite3_bind_int(stmt, 4, o.estimatedTime);
        sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, o.day);
        sqlite3_bind_int(stmt, 7, o.week);
        sqlite3_bind_int(stmt, 8, o.month);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // get order_id
        int order_id = sqlite3_last_insert_rowid(db);
        for (int i = 0; i < o.foodCount; ++i) {
            sqlite3_stmt* stmt2;
            const char* sql2 = "INSERT INTO OrderItems (order_id, food_name, quantity, price_at_time, preparation_time) VALUES (?, ?, ?, ?, ?)";
            sqlite3_prepare_v2(db, sql2, -1, &stmt2, nullptr);
            sqlite3_bind_int(stmt2, 1, order_id);
            sqlite3_bind_text(stmt2, 2, o.foods[i].c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt2, 3, o.quantities[i]);
            // price_at_time needs to be fetched from Foods table
            float price = 0.0f;
            sqlite3_stmt* stmt_price;
            sqlite3_prepare_v2(db, "SELECT price FROM Foods WHERE name = ?", -1, &stmt_price, nullptr);
            sqlite3_bind_text(stmt_price, 1, o.foods[i].c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt_price) == SQLITE_ROW)
                price = (float)sqlite3_column_double(stmt_price, 0);
            sqlite3_finalize(stmt_price);
            sqlite3_bind_double(stmt2, 4, price);
            sqlite3_bind_int(stmt2, 5, o.preparationTimes[i]);
            sqlite3_step(stmt2);
            sqlite3_finalize(stmt2);
        }
    }
}

int getQueueWaitTime() {
    // total preparation time of all pending orders (not picked up)
    int total = 0;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT SUM(preparation_time * quantity) FROM OrderItems WHERE order_id IN (SELECT order_id FROM Orders WHERE status != 'picked_up')";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        total = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return total;
}

string loadAdminPass() {
    string pw = DEFAULT_ADMIN_PASS;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT admin_password FROM SystemInfo WHERE id = 1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        pw = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);
    return pw;
}

void saveAdminPass(const string& pw) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE SystemInfo SET admin_password = ? WHERE id = 1";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, pw.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void saveDayInfo() {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE SystemInfo SET current_day = ?, current_week = ?, current_month = ? WHERE id = 1";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, currentDay);
    sqlite3_bind_int(stmt, 2, currentWeek);
    sqlite3_bind_int(stmt, 3, currentMonth);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void loadDayInfo() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT current_day, current_week, current_month FROM SystemInfo WHERE id = 1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        currentDay = sqlite3_column_int(stmt, 0);
        currentWeek = sqlite3_column_int(stmt, 1);
        currentMonth = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
}

void saveCounter() {
    int lastOrder = 0;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT MAX(order_number) FROM Orders";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        lastOrder = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    sqlite3_stmt* stmt2;
    const char* sql2 = "UPDATE SystemInfo SET last_order_number = ? WHERE id = 1";
    sqlite3_prepare_v2(db, sql2, -1, &stmt2, nullptr);
    sqlite3_bind_int(stmt2, 1, lastOrder);
    sqlite3_step(stmt2);
    sqlite3_finalize(stmt2);
}

void loadCounter() {
    // not used, but we keep for compatibility; order numbers are generated based on max+1
}

// ============================================================
// USER FUNCTIONS
// ============================================================
bool validateStudentID(const string& id) {
    if (id.length() != 9) return false;
    for (char c : id) if (!isdigit(c)) return false;
    return true;
}

bool validateName(const string& name) {
    if (name.empty() || name.length() > 25) return false;
    for (char c : name) if (!isalpha(c) && c != ' ') return false;
    return true;
}

bool validatePhone(const string& phone) {
    if (phone.length() != 10) return false;
    for (char c : phone) if (!isdigit(c)) return false;
    return true;
}

bool idExists(const string& id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM Users WHERE studentID = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

int findUser(const vector<User>& users, const string& id) {
    for (size_t i = 0; i < users.size(); ++i)
        if (users[i].studentID == id) return i;
    return -1;
}

void registerUser() {
    printHeader("User Registration");
    string id, name, phone, pass;
    do {
        id = getLineInput("  Student ID (9 digits): ");
        if (!validateStudentID(id)) cout << "  Invalid ID (9 digits).\n";
        else if (idExists(id)) cout << "  ID already exists.\n";
        else break;
    } while (true);
    do {
        name = getLineInput("  Full Name (max 25 chars): ");
        if (!validateName(name)) cout << "  Name can only contain letters and spaces.\n";
        else break;
    } while (true);
    do {
        phone = getLineInput("  Phone (10 digits): ");
        if (!validatePhone(phone)) cout << "  Phone must be 10 digits.\n";
        else break;
    } while (true);
    pass = getLineInput("  Password: ");
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Users (studentID, name, phone, password) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, phone.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pass.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "\n  Registration successful!\n";
    pressEnter();
}

int loginUser(vector<User>& users) {
    printHeader("User Login");
    string id, pass;
    id = getLineInput("  Student ID: ");
    pass = getLineInput("  Password: ");
    sqlite3_stmt* stmt;
    const char* sql = "SELECT user_id, name, phone FROM Users WHERE studentID = ? AND password = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int uid = sqlite3_column_int(stmt, 0);
        string name = (const char*)sqlite3_column_text(stmt, 1);
        string phone = (const char*)sqlite3_column_text(stmt, 2);
        sqlite3_finalize(stmt);
        users = loadUsers();
        for (size_t i = 0; i < users.size(); ++i)
            if (users[i].studentID == id) return i;
        // if not found in vector, push new
        User u;
        strcpy(u.studentID, id.c_str());
        strcpy(u.name, name.c_str());
        strcpy(u.phone, phone.c_str());
        u.password = pass;
        users.push_back(u);
        return users.size() - 1;
    }
    sqlite3_finalize(stmt);
    cout << "  Invalid credentials.\n";
    pressEnter();
    return -1;
}

bool viewCafeteriaMenu() {
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << "  Menu is empty.\n";
        return false;
    }
    cout << "\n  === CAFETERIA MENU ===\n";
    cout << left << setw(5) << "No." << setw(30) << "Item" << setw(10) << "Price" << "Time(min)\n";
    for (size_t i = 0; i < foods.size(); ++i) {
        cout << "  " << i+1 << ". " << setw(28) << foods[i].name << "$" << setw(9) << fixed << setprecision(2) << foods[i].price << foods[i].preparationTime << "\n";
    }
    return true;
}

void orderFood(const User& user) {
    vector<Food> foods = loadFoods();
    if (foods.empty()) {
        cout << "  No food items available. Please contact admin.\n";
        pressEnter();
        return;
    }
    Order newOrder;
    newOrder.studentID = user.studentID;
    newOrder.customerName = user.name;
    newOrder.customerPhone = user.phone;
    newOrder.day = currentDay;
    newOrder.week = currentWeek;
    newOrder.month = currentMonth;
    newOrder.prepared = false;
    newOrder.pickedUp = false;
    newOrder.foodCount = 0;
    newOrder.totalPrice = 0;
    newOrder.estimatedTime = 0;

    while (true) {
        viewCafeteriaMenu();
        cout << "\n  Enter item number to add (0 to finish): ";
        int choice = getInt("  Choice: ", 0, foods.size());
        if (choice == 0) break;
        int qty = getInt("  Quantity: ", 1, 100);
        Food& f = foods[choice-1];
        newOrder.foods[newOrder.foodCount] = f.name;
        newOrder.quantities[newOrder.foodCount] = qty;
        newOrder.preparationTimes[newOrder.foodCount] = f.preparationTime;
        newOrder.totalPrice += f.price * qty;
        newOrder.estimatedTime += f.preparationTime * qty;
        newOrder.foodCount++;
        if (newOrder.foodCount >= 10) {
            cout << "  Maximum 10 items reached.\n";
            break;
        }
    }
    if (newOrder.foodCount == 0) {
        cout << "  No items ordered.\n";
        pressEnter();
        return;
    }
    // assign order number
    int nextOrderNum = 1;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT MAX(order_number) FROM Orders";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        nextOrderNum = sqlite3_column_int(stmt, 0) + 1;
    sqlite3_finalize(stmt);
    newOrder.orderNumber = nextOrderNum;
    // insert into DB
    // find user_id
    int user_id = -1;
    sqlite3_stmt* stmt2;
    const char* sql2 = "SELECT user_id FROM Users WHERE studentID = ?";
    sqlite3_prepare_v2(db, sql2, -1, &stmt2, nullptr);
    sqlite3_bind_text(stmt2, 1, user.studentID, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt2) == SQLITE_ROW)
        user_id = sqlite3_column_int(stmt2, 0);
    sqlite3_finalize(stmt2);
    if (user_id == -1) {
        cout << "  User not found. Please login again.\n";
        pressEnter();
        return;
    }
    sqlite3_stmt* stmt3;
    const char* sql3 = "INSERT INTO Orders (order_number, user_id, total_price, estimated_time, status, day, week, month) VALUES (?, ?, ?, ?, 'pending', ?, ?, ?)";
    sqlite3_prepare_v2(db, sql3, -1, &stmt3, nullptr);
    sqlite3_bind_int(stmt3, 1, newOrder.orderNumber);
    sqlite3_bind_int(stmt3, 2, user_id);
    sqlite3_bind_double(stmt3, 3, newOrder.totalPrice);
    sqlite3_bind_int(stmt3, 4, newOrder.estimatedTime);
    sqlite3_bind_int(stmt3, 5, currentDay);
    sqlite3_bind_int(stmt3, 6, currentWeek);
    sqlite3_bind_int(stmt3, 7, currentMonth);
    sqlite3_step(stmt3);
    sqlite3_finalize(stmt3);
    int order_id = sqlite3_last_insert_rowid(db);
    for (int i = 0; i < newOrder.foodCount; ++i) {
        sqlite3_stmt* stmt4;
        const char* sql4 = "INSERT INTO OrderItems (order_id, food_name, quantity, price_at_time, preparation_time) VALUES (?, ?, ?, ?, ?)";
        sqlite3_prepare_v2(db, sql4, -1, &stmt4, nullptr);
        sqlite3_bind_int(stmt4, 1, order_id);
        sqlite3_bind_text(stmt4, 2, newOrder.foods[i].c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt4, 3, newOrder.quantities[i]);
        // get current price
        float price = 0;
        sqlite3_stmt* stmt5;
        sqlite3_prepare_v2(db, "SELECT price FROM Foods WHERE name = ?", -1, &stmt5, nullptr);
        sqlite3_bind_text(stmt5, 1, newOrder.foods[i].c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt5) == SQLITE_ROW)
            price = (float)sqlite3_column_double(stmt5, 0);
        sqlite3_finalize(stmt5);
        sqlite3_bind_double(stmt4, 4, price);
        sqlite3_bind_int(stmt4, 5, newOrder.preparationTimes[i]);
        sqlite3_step(stmt4);
        sqlite3_finalize(stmt4);
    }
    cout << "\n  Order placed successfully!\n";
    cout << "  Order Number: " << newOrder.orderNumber << "\n";
    cout << "  Estimated total time: " << newOrder.estimatedTime + getQueueWaitTime() << " minutes\n";
    pressEnter();
}

void viewCurrentOrderStatus(const string& studentID) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT order_number, status, total_price, estimated_time FROM Orders WHERE user_id = (SELECT user_id FROM Users WHERE studentID = ?) AND status != 'picked_up' ORDER BY order_number DESC LIMIT 1";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, studentID.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int onum = sqlite3_column_int(stmt, 0);
        string status = (const char*)sqlite3_column_text(stmt, 1);
        float price = (float)sqlite3_column_double(stmt, 2);
        int est = sqlite3_column_int(stmt, 3);
        cout << "  Order #" << onum << " | Status: " << status << " | Total: $" << fixed << setprecision(2) << price << " | Est. time: " << est << " min\n";
    } else {
        cout << "  No active orders.\n";
    }
    sqlite3_finalize(stmt);
}

void viewOrderHistory(const string& studentID) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT order_number, status, total_price, day, week, month FROM Orders WHERE user_id = (SELECT user_id FROM Users WHERE studentID = ?) ORDER BY order_number DESC";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, studentID.c_str(), -1, SQLITE_STATIC);
    bool hasAny = false;
    cout << "\n  === ORDER HISTORY ===\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hasAny = true;
        int onum = sqlite3_column_int(stmt, 0);
        string status = (const char*)sqlite3_column_text(stmt, 1);
        float price = (float)sqlite3_column_double(stmt, 2);
        int d = sqlite3_column_int(stmt, 3);
        int w = sqlite3_column_int(stmt, 4);
        int m = sqlite3_column_int(stmt, 5);
        cout << "  #" << onum << " | $" << fixed << setprecision(2) << price << " | " << status << " | Day " << d << ", Week " << w << ", Month " << m << "\n";
    }
    if (!hasAny) cout << "  No orders found.\n";
    sqlite3_finalize(stmt);
}

void viewOrdersMenu(const string& studentID) {
    printHeader("Order Menu");
    cout << "  1. View Current Order Status\n  2. View Order History\n";
    int ch = getInt("  Choice: ", 1, 2);
    if (ch == 1) viewCurrentOrderStatus(studentID);
    else viewOrderHistory(studentID);
    pressEnter();
}

void userChangePassword(User& user, vector<User>& users, int idx) {
    printHeader("Change Password");
    string old = getLineInput("  Enter current password: ");
    if (old != user.password) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }
    string newp = getLineInput("  New password: ");
    string confirm = getLineInput("  Confirm password: ");
    if (newp != confirm) {
        cout << "  Passwords do not match.\n";
        pressEnter();
        return;
    }
    user.password = newp;
    // update DB
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE Users SET password = ? WHERE studentID = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, newp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.studentID, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    users[idx] = user;
    cout << "  Password changed successfully.\n";
    pressEnter();
}

void userMenu(int userIdx) {
    vector<User> users = loadUsers();
    User currentUser = users[userIdx];
    while (true) {
        printHeader("User Menu - Welcome " + string(currentUser.name));
        cout << "  1. View Cafeteria Menu\n  2. Place Order\n  3. View Orders\n  4. Change Password\n  5. Logout\n";
        int ch = getInt("  Choice: ", 1, 5);
        if (ch == 1) viewCafeteriaMenu();
        else if (ch == 2) orderFood(currentUser);
        else if (ch == 3) viewOrdersMenu(currentUser.studentID);
        else if (ch == 4) userChangePassword(currentUser, users, userIdx);
        else break;
        if (ch != 5) pressEnter();
    }
}

void userPartMenu() {
    vector<User> users = loadUsers();
    while (true) {
        printHeader("User Section");
        cout << "  1. Register\n  2. Login\n  3. Back to Main\n";
        int ch = getInt("  Choice: ", 1, 3);
        if (ch == 1) registerUser();
        else if (ch == 2) {
            int idx = loginUser(users);
            if (idx >= 0) userMenu(idx);
        } else break;
    }
}

// ============================================================
// ADMIN FUNCTIONS
// ============================================================
void adminInsertFood() {
    printHeader("Add Food Item");
    string name = getLineInput("  Food name: ");
    float price = (float)getInt("  Price (cents, e.g., 550 for $5.50): ", 0, 10000) / 100.0f;
    int time = getInt("  Preparation time (minutes): ", 1, 120);
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Foods (name, price, preparation_time) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, price);
    sqlite3_bind_int(stmt, 3, time);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "  Food added.\n";
    pressEnter();
}

void adminUpdateFoodPrice() {
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods.\n"; pressEnter(); return; }
    viewCafeteriaMenu();
    int idx = getInt("  Enter item number to update price: ", 1, foods.size()) - 1;
    float newPrice = (float)getInt("  New price (cents): ", 0, 10000) / 100.0f;
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE Foods SET price = ? WHERE name = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, newPrice);
    sqlite3_bind_text(stmt, 2, foods[idx].name.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "  Price updated.\n";
    pressEnter();
}

void adminUpdatePrepTime() {
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods.\n"; pressEnter(); return; }
    viewCafeteriaMenu();
    int idx = getInt("  Enter item number to update preparation time: ", 1, foods.size()) - 1;
    int newTime = getInt("  New preparation time (minutes): ", 1, 120);
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE Foods SET preparation_time = ? WHERE name = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, newTime);
    sqlite3_bind_text(stmt, 2, foods[idx].name.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "  Preparation time updated.\n";
    pressEnter();
}

void adminDeleteFood() {
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods.\n"; pressEnter(); return; }
    viewCafeteriaMenu();
    int idx = getInt("  Enter item number to delete: ", 1, foods.size()) - 1;
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM Foods WHERE name = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, foods[idx].name.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "  Food deleted.\n";
    pressEnter();
}

void adminViewFoods() { viewCafeteriaMenu(); pressEnter(); }

void adminEditMenu() {
    while (true) {
        printHeader("Edit Menu");
        cout << "  1. Add Food\n  2. Update Price\n  3. Update Prep Time\n  4. Delete Food\n  5. View All Foods\n  6. Back\n";
        int ch = getInt("  Choice: ", 1, 6);
        if (ch == 1) adminInsertFood();
        else if (ch == 2) adminUpdateFoodPrice();
        else if (ch == 3) adminUpdatePrepTime();
        else if (ch == 4) adminDeleteFood();
        else if (ch == 5) adminViewFoods();
        else break;
        if (ch != 5 && ch != 6) pressEnter();
    }
}

void adminViewWaitingOrders() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT order_number, studentID, name, total_price, estimated_time FROM Orders JOIN Users ON Orders.user_id = Users.user_id WHERE status = 'pending' ORDER BY order_number";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n  === WAITING ORDERS (Pending) ===\n";
    bool any = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        any = true;
        int onum = sqlite3_column_int(stmt, 0);
        string sid = (const char*)sqlite3_column_text(stmt, 1);
        string name = (const char*)sqlite3_column_text(stmt, 2);
        float price = (float)sqlite3_column_double(stmt, 3);
        int est = sqlite3_column_int(stmt, 4);
        cout << "  #" << onum << " | " << sid << " | " << name << " | $" << fixed << setprecision(2) << price << " | Est: " << est << " min\n";
    }
    if (!any) cout << "  No pending orders.\n";
    sqlite3_finalize(stmt);
    pressEnter();
}

void adminMarkPrepared() {
    adminViewWaitingOrders();
    int onum = getInt("  Enter order number to mark as prepared: ", 1, 999999);
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE Orders SET status = 'prepared' WHERE order_number = ? AND status = 'pending'";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, onum);
    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0)
        cout << "  Order #" << onum << " marked as prepared.\n";
    else
        cout << "  Order not found or already prepared.\n";
    sqlite3_finalize(stmt);
    pressEnter();
}

void adminViewPreparedUnpicked() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT order_number, studentID, name, total_price FROM Orders JOIN Users ON Orders.user_id = Users.user_id WHERE status = 'prepared'";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n  === PREPARED ORDERS (Awaiting Pickup) ===\n";
    bool any = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        any = true;
        int onum = sqlite3_column_int(stmt, 0);
        string sid = (const char*)sqlite3_column_text(stmt, 1);
        string name = (const char*)sqlite3_column_text(stmt, 2);
        float price = (float)sqlite3_column_double(stmt, 3);
        cout << "  #" << onum << " | " << sid << " | " << name << " | $" << fixed << setprecision(2) << price << "\n";
    }
    if (!any) cout << "  No prepared orders.\n";
    sqlite3_finalize(stmt);
    pressEnter();
}

void adminMarkPickedUp() {
    adminViewPreparedUnpicked();
    int onum = getInt("  Enter order number to mark as picked up: ", 1, 999999);
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE Orders SET status = 'picked_up' WHERE order_number = ? AND status = 'prepared'";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, onum);
    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0)
        cout << "  Order #" << onum << " marked as picked up.\n";
    else
        cout << "  Order not found or not prepared.\n";
    sqlite3_finalize(stmt);
    pressEnter();
}

void adminViewCustomers() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT studentID, name, phone, COUNT(Orders.order_id) as order_count FROM Users LEFT JOIN Orders ON Users.user_id = Orders.user_id GROUP BY Users.user_id";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n  === CUSTOMER LIST ===\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        string sid = (const char*)sqlite3_column_text(stmt, 0);
        string name = (const char*)sqlite3_column_text(stmt, 1);
        string phone = (const char*)sqlite3_column_text(stmt, 2);
        int cnt = sqlite3_column_int(stmt, 3);
        cout << "  " << sid << " | " << name << " | " << phone << " | Orders: " << cnt << "\n";
    }
    sqlite3_finalize(stmt);
    pressEnter();
}

void generateReport(const string& label, int filterDay, int filterWeek, int filterMonth) {
    string condition;
    if (filterDay != -1) condition = " day = " + to_string(filterDay) + " AND week = " + to_string(filterWeek) + " AND month = " + to_string(filterMonth);
    else if (filterWeek != -1) condition = " week = " + to_string(filterWeek) + " AND month = " + to_string(filterMonth);
    else if (filterMonth != -1) condition = " month = " + to_string(filterMonth);
    else condition = "1=1";
    string sqlStr = "SELECT order_number, studentID, name, total_price, status, day, week, month FROM Orders JOIN Users ON Orders.user_id = Users.user_id WHERE " + condition;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sqlStr.c_str(), -1, &stmt, nullptr);
    cout << "\n  === " << label << " REPORT ===\n";
    float total = 0;
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
        total += (float)sqlite3_column_double(stmt, 3);
        cout << "  #" << sqlite3_column_int(stmt, 0) << " | " << (const char*)sqlite3_column_text(stmt, 1) << " | " << (const char*)sqlite3_column_text(stmt, 2) << " | $" << fixed << setprecision(2) << sqlite3_column_double(stmt, 3) << " | " << (const char*)sqlite3_column_text(stmt, 4) << "\n";
    }
    cout << "  Total orders: " << count << " | Total revenue: $" << fixed << setprecision(2) << total << "\n";
    sqlite3_finalize(stmt);
    pressEnter();
}

void adminReports() {
    while (true) {
        printHeader("Reports");
        cout << "  1. Today's Report\n  2. This Week's Report\n  3. This Month's Report\n  4. All-Time Report\n  5. Back\n";
        int ch = getInt("  Choice: ", 1, 5);
        if (ch == 1) generateReport("TODAY", currentDay, currentWeek, currentMonth);
        else if (ch == 2) generateReport("THIS WEEK", -1, currentWeek, currentMonth);
        else if (ch == 3) generateReport("THIS MONTH", -1, -1, currentMonth);
        else if (ch == 4) generateReport("ALL TIME", -1, -1, -1);
        else break;
    }
}

void adminChangeDay() {
    printHeader("Change System Day/Week/Month");
    int newDay = getInt("  Enter new day number: ", 1, 365);
    int newWeek = getInt("  Enter new week number: ", 1, 52);
    int newMonth = getInt("  Enter new month number: ", 1, 12);
    currentDay = newDay;
    currentWeek = newWeek;
    currentMonth = newMonth;
    saveDayInfo();
    cout << "  System date updated.\n";
    pressEnter();
}

void adminChangePassword() {
    printHeader("Change Admin Password");
    string old = getLineInput("  Enter current admin password: ");
    if (old != loadAdminPass()) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }
    string newp = getLineInput("  New password: ");
    string confirm = getLineInput("  Confirm: ");
    if (newp != confirm) {
        cout << "  Passwords do not match.\n";
        pressEnter();
        return;
    }
    saveAdminPass(newp);
    cout << "  Admin password changed.\n";
    pressEnter();
}

void adminMenu() {
    while (true) {
        printHeader("Admin Menu");
        cout << "  1. Edit Menu\n  2. View Waiting Orders\n  3. Mark Order as Prepared\n  4. View Prepared Orders (Unpicked)\n  5. Mark as Picked Up\n  6. View All Customers\n  7. Generate Reports\n  8. Change System Day/Week/Month\n  9. Change Admin Password\n  10. Logout\n";
        int ch = getInt("  Choice: ", 1, 10);
        if (ch == 1) adminEditMenu();
        else if (ch == 2) adminViewWaitingOrders();
        else if (ch == 3) adminMarkPrepared();
        else if (ch == 4) adminViewPreparedUnpicked();
        else if (ch == 5) adminMarkPickedUp();
        else if (ch == 6) adminViewCustomers();
        else if (ch == 7) adminReports();
        else if (ch == 8) adminChangeDay();
        else if (ch == 9) adminChangePassword();
        else break;
    }
}

void adminLogin() {
    printHeader("Admin Login");
    string pass = getLineInput("  Admin password: ");
    if (pass == loadAdminPass()) {
        cout << "  Access granted.\n";
        adminMenu();
    } else {
        cout << "  Invalid password.\n";
        pressEnter();
    }
}

// ============================================================
// MAIN (already defined, but we keep it)
// ============================================================
int main() {
    if (!initializeDatabase()) {
        cerr << "Failed to initialize database. Exiting.\n";
        return 1;
    }
    loadDayInfo();
    loadCounter();

    while (true) {
        printHeader("Main Menu");
        cout << "  1. User\n  2. Admin\n  3. Exit\n";
        int ch = getInt("  Choice: ", 1, 3);
        if (ch == 1) userPartMenu();
        else if (ch == 2) adminLogin();
        else {
            cout << "\n  Thank you for using Central Cafeteria. Goodbye!\n\n";
            return 0;
        }
    }
}
