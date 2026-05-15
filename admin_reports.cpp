#include "cafeteria.h"
#include <sqlite3.h>

extern sqlite3* db;

void adminViewCustomers() {
    printHeader("Customer Information");
    vector<User> users = loadUsers();
    if (users.empty()) {
        cout << "  No registered users.\n";
        pressEnter();
        return;
    }
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
    sqlite3_stmt* stmt = nullptr;
    float totalRevenue = 0;
    int totalOrders = 0, pickedUp = 0, unpicked = 0;

    string sql = "SELECT o.total_price, o.status, oi.food_name, oi.quantity "
                 "FROM Orders o LEFT JOIN OrderItems oi ON o.order_number = oi.order_id "
                 "WHERE 1=1";

    if (filterDay != -1) sql += " AND o.day = " + to_string(filterDay);
    if (filterWeek != -1) sql += " AND o.week = " + to_string(filterWeek);
    if (filterMonth != -1) sql += " AND o.month = " + to_string(filterMonth);

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "  Database error.\n";
        pressEnter();
        return;
    }

    vector<string> foodNames;
    vector<int> foodSales;

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
        cout << "  1. Daily Report (Day " << currentDay << ")\n";
        cout << "  2. Weekly Report (Week " << currentWeek << ")\n";
        cout << "  3. Monthly Report (Month " << currentMonth << ")\n";
        cout << "  4. Back\n";
        int ch = getInt("  Choice: ", 1, 4);
        if (ch == 1) generateReport("Daily", currentDay, -1, -1);
        else if (ch == 2) generateReport("Weekly", -1, currentWeek, -1);
        else if (ch == 3) generateReport("Monthly", -1, -1, currentMonth);
        else return;
    }
}

void adminChangeDay() {
    printHeader("Change Day");
    cout << "  Current: Day " << currentDay << " Week " << currentWeek << " Month " << currentMonth << "\n\n";
    cout << "  Advance to next day?\n";
    cout << "  1. Yes\n  2. No\n";
    int ch = getInt("  Choice: ", 1, 2);
    if (ch == 2) return;

    currentDay++;
    if (currentDay % 7 == 1 && currentDay > 1) currentWeek++;
    if (currentDay % 30 == 1 && currentDay > 1) currentMonth++;

    orderCounter = 0;
    saveDayInfo();
    saveCounter();

    cout << "  Day advanced. Now: Day " << currentDay << " Week " << currentWeek << " Month " << currentMonth << "\n";
    pressEnter();
}

void adminChangePassword() {
    printHeader("Admin Change Password");
    string adminPw = loadAdminPass();
    string oldPw = getLineInput("  Enter current password: ");
    if (oldPw != adminPw) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }

    string newPw, confirm;
    while (true) {
        newPw = getLineInput("  Enter new password: ");
        confirm = getLineInput("  Confirm new password: ");
        if (newPw.empty()) { cout << "  Password cannot be empty.\n"; continue; }
        if (newPw != confirm) { cout << "  Passwords do not match.\n"; continue; }
        break;
    }
    saveAdminPass(newPw);
    cout << "  Password changed successfully.\n";
    pressEnter();
}

void adminMenu() {
    while (true) {
        printHeader("Admin Menu");
        cout << "  Day " << currentDay << " | Week " << currentWeek << " | Month " << currentMonth << "\n\n";
        cout << "  1. Edit Menu\n";
        cout << "  2. View Waiting Orders\n";
        cout << "  3. View Prepared Orders\n";
        cout << "  4. Mark Order as Prepared\n";
        cout << "  5. Mark Order as Picked Up\n";
        cout << "  6. View Customers\n";
        cout << "  7. Reports\n";
        cout << "  8. Change Day\n";
        cout << "  9. Change Password\n";
        cout << "  10. Logout\n";
        int ch = getInt("  Choice: ", 1, 10);
        if (ch == 6) adminViewCustomers();
        else if (ch == 7) adminReports();
        else if (ch == 8) adminChangeDay();
        else if (ch == 9) adminChangePassword();
        else if (ch == 10) {
            cout << "  Admin logged out.\n";
            pressEnter();
            return;
        }
        else {
            cout << "  Feature handled by other team members.\n";
            pressEnter();
        }
    }
}

void adminLogin() {
    printHeader("Admin Login");
    string adminPw = loadAdminPass();
    string entered = getLineInput("  Enter Admin Password: ");
    if (entered != adminPw) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }
    cout << "  Access granted.\n";
    pressEnter();
    adminMenu();
}