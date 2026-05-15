#include "cafeteria.h"
#include <sqlite3.h>
#include <cstdlib>

// ========== GLOBAL VARIABLES DEFINITION ==========
sqlite3* db = nullptr;
int currentDay = 1;
int currentWeek = 1;
const string CAFETERIA_NAME = "Central Cafeteria";
const string DEFAULT_ADMIN_PASS = "admin123";
User currentUser = {0, "", "", "", "", 0, 0, 0.0};

// ========== UTILITY FUNCTION IMPLEMENTATIONS ==========
string toUpperStr(string str) {
    for (char &c : str) c = toupper(c);
    return str;
}

void pressEnter() {
    cout << "\nPress Enter to continue...";
    cin.ignore();
    cin.get();
}

void printLine(char ch, int length) {
    for (int i = 0; i < length; i++) cout << ch;
    cout << endl;
}

void printHeader(const string& title) {
    printLine('=', 60);
    cout << "\t" << title << endl;
    printLine('=', 60);
}

int getInt(const string& prompt) {
    int value;
    cout << prompt;
    while (!(cin >> value)) {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Invalid input! Please enter a number: ";
    }
    cin.ignore();
    return value;
}

string getLineInput(const string& prompt) {
    string input;
    cout << prompt;
    getline(cin, input);
    return input;
}

// ========== DATABASE FUNCTIONS ==========
bool initDatabase() {
    int rc = sqlite3_open("central_cafeteria.db", &db);
    if (rc) {
        cout << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return false;
    }
    createTables();
    return true;
}

void createTables() {
    char *errMsg = nullptr;

    // Users table
    string sqlUsers = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "student_id TEXT UNIQUE, "
        "name TEXT, "
        "phone TEXT, "
        "password TEXT, "
        "is_admin INTEGER, "
        "total_orders INTEGER DEFAULT 0, "
        "total_spent REAL DEFAULT 0);";

    // Foods table
    string sqlFoods = 
        "CREATE TABLE IF NOT EXISTS foods ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "price REAL, "
        "prep_time INTEGER);";

    // Orders table
    string sqlOrders = 
        "CREATE TABLE IF NOT EXISTS orders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER, "
        "food_id INTEGER, "
        "quantity INTEGER, "
        "order_time TEXT, "
        "status TEXT, "
        "day INTEGER, "
        "week INTEGER, "
        "FOREIGN KEY(user_id) REFERENCES users(id), "
        "FOREIGN KEY(food_id) REFERENCES foods(id));";

    sqlite3_exec(db, sqlUsers.c_str(), 0, 0, &errMsg);
    sqlite3_exec(db, sqlFoods.c_str(), 0, 0, &errMsg);
    sqlite3_exec(db, sqlOrders.c_str(), 0, 0, &errMsg);

    // Insert default admin if not exists
    string checkAdmin = "SELECT COUNT(*) FROM users WHERE is_admin = 1;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, checkAdmin.c_str(), -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int adminCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (adminCount == 0) {
        string insertAdmin = "INSERT INTO users (student_id, name, phone, password, is_admin) "
                             "VALUES ('ADMIN', 'Administrator', '0000000000', '" + DEFAULT_ADMIN_PASS + "', 1);";
        sqlite3_exec(db, insertAdmin.c_str(), 0, 0, &errMsg);
    }

    // Insert sample foods if table empty
    string checkFoods = "SELECT COUNT(*) FROM foods;";
    sqlite3_prepare_v2(db, checkFoods.c_str(), -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int foodCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (foodCount == 0) {
        vector<pair<string, pair<double, int>>> sampleFoods = {
            {"Rice & Chicken", {5.50, 10}},
            {"Spaghetti Bolognese", {4.75, 12}},
            {"Club Sandwich", {3.50, 7}},
            {"Fresh Juice", {2.00, 3}},
            {"Coffee", {1.50, 2}}
        };
        for (auto &f : sampleFoods) {
            string sql = "INSERT INTO foods (name, price, prep_time) VALUES ('" + 
                         f.first + "', " + to_string(f.second.first) + ", " + 
                         to_string(f.second.second) + ");";
            sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        }
    }
}

// ========== MAIN FUNCTION ==========
int main() {
    if (!initDatabase()) {
        cout << "Failed to initialize database.\n";
        return 1;
    }

    cout << "\n\n";
    printHeader("WELCOME TO " + CAFETERIA_NAME);
    cout << "\n\t   Your Daily Meal Solution\n\n";

    while (true) {
        cout << "\n";
        printLine('-', 50);
        cout << "1. User Login\n";
        cout << "2. User Registration\n";
        cout << "3. Admin Login\n";
        cout << "4. Exit\n";
        printLine('-', 50);
        cout << "Day: " << currentDay << " | Week: " << currentWeek << endl;

        int choice = getInt("Enter choice: ");

        switch (choice) {
            case 1:
                if (loginUser()) {
                    userMenu();
                }
                break;
            case 2:
                registerUser();
                break;
            case 3:
                if (adminLogin()) {
                    adminMenu();
                }
                break;
            case 4:
                cout << "Thank you for using " << CAFETERIA_NAME << "!\n";
                sqlite3_close(db);
                return 0;
            default:
                cout << "Invalid choice!\n";
        }
    }

    return 0;
}
