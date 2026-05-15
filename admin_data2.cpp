#include "cafeteria.h"
#include <sqlite3.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <unistd.h>

using namespace std;

extern sqlite3* db;
extern int currentDay;
extern int currentWeek;

void adminEditMenu() {
while (true) {
        cout << "\n";
        printHeader("EDIT MENU");
        cout << "1. View All Foods\n";
        cout << "2. Add New Food\n";
        cout << "3. Update Food Price\n";
        cout << "4. Update Preparation Time\n";
        cout << "5. Delete Food\n";
        cout << "6. Back to Admin Menu\n";
        printLine();
        cout << "Enter choice: ";

        int choice = getInt();

        switch (choice) {
            case 1:
                adminViewFoods();
                break;
            case 2:
                adminInsertFood();
                break;
            case 3:
                adminUpdateFoodPrice();
                break;
            case 4:
                adminUpdatePrepTime();
                break;
            case 5:
                adminDeleteFood();
                break;
            case 6:
                return;
            default:
                cout << "Invalid choice! Try again.\n";
        }
    }
}

// ============ ORDER MANAGEMENT FUNCTIONS (ADMIN) ============

void adminViewWaitingOrders() {
    const char* sql = "SELECT o.id, u.name, f.name, o.quantity, o.order_time, o.status, o.queue_position "
                     "FROM orders o "
                     "JOIN users u ON o.user_id = u.id "
                     "JOIN foods f ON o.food_id = f.id "
                     "WHERE o.day = ? AND o.status = 'waiting' "
                     "ORDER BY o.queue_position";

    sqlite3_stmt* stmt;

    cout << "\n";
    printHeader("WAITING ORDERS");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, currentDay);

        bool hasOrders = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            hasOrders = true;
            int id = sqlite3_column_int(stmt, 0);
            const char* userName = (const char*)sqlite3_column_text(stmt, 1);
            const char* foodName = (const char*)sqlite3_column_text(stmt, 2);
            int quantity = sqlite3_column_int(stmt, 3);
            const char* orderTime = (const char*)sqlite3_column_text(stmt, 4);
            int queuePos = sqlite3_column_int(stmt, 6);

            cout << "Order #" << id << " | Queue: " << queuePos << "\n";
            cout << "  Customer: " << userName << "\n";
            cout << "  Item: " << foodName << " x" << quantity << "\n";
            cout << "  Time: " << orderTime << "\n";
            printLine();
        }

        if (!hasOrders) {
            cout << "No waiting orders.\n";
        }
        sqlite3_finalize(stmt);
    }
}

void adminMarkPrepared() {
    adminViewWaitingOrders();

    int orderId;
    cout << "\nEnter order ID to mark as prepared: ";
    cin >> orderId;

    const char* checkSql = "SELECT status FROM orders WHERE id = ? AND day = ? AND status = 'waiting'";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, orderId);
        sqlite3_bind_int(checkStmt, 2, currentDay);

        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* updateSql = "UPDATE orders SET status = 'prepared' WHERE id = ?";
            sqlite3_stmt* updateStmt;

            if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(updateStmt, 1, orderId);

                if (sqlite3_step(updateStmt) == SQLITE_DONE) {
                    cout << "\n✓ Order #" << orderId << " marked as prepared!\n";
                } else {
                    cout << "✗ Failed to update order!\n";
                }
                sqlite3_finalize(updateStmt);
            }
        } else {
            cout << "Order not found or already processed!\n";
        }
        sqlite3_finalize(checkStmt);
    }
}
void adminViewPreparedUnpicked() {
    const char* sql = "SELECT o.id, u.name, f.name, o.quantity, o.order_time "
                     "FROM orders o "
                     "JOIN users u ON o.user_id = u.id "
                     "JOIN foods f ON o.food_id = f.id "
                     "WHERE o.day = ? AND o.status = 'prepared' "
                     "ORDER BY o.id";

    sqlite3_stmt* stmt;

    cout << "\n";
    printHeader("PREPARED ORDERS (AWAITING PICKUP)");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, currentDay);

        bool hasOrders = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            hasOrders = true;
            int id = sqlite3_column_int(stmt, 0);
            const char* userName = (const char*)sqlite3_column_text(stmt, 1);
            const char* foodName = (const char*)sqlite3_column_text(stmt, 2);
            int quantity = sqlite3_column_int(stmt, 3);
            const char* orderTime = (const char*)sqlite3_column_text(stmt, 4);

            cout << "Order #" << id << " | Customer: " << userName << "\n";
            cout << "  Item: " << foodName << " x" << quantity << "\n";
            cout << "  Time: " << orderTime << "\n";
            printLine();
        }

        if (!hasOrders) {
            cout << "No prepared orders awaiting pickup.\n";
        }
        sqlite3_finalize(stmt);
    }
}

void adminMarkPickedUp() {
    adminViewPreparedUnpicked();

    int orderId;
    cout << "\nEnter order ID to mark as picked up: ";
    cin >> orderId;

    const char* checkSql = "SELECT status FROM orders WHERE id = ? AND day = ? AND status = 'prepared'";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, orderId);
        sqlite3_bind_int(checkStmt, 2, currentDay);

        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* updateSql = "UPDATE orders SET status = 'completed' WHERE id = ?";
            sqlite3_stmt* updateStmt;

            if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(updateStmt, 1, orderId);

                if (sqlite3_step(updateStmt) == SQLITE_DONE) {
                    cout << "\n✓ Order #" << orderId << " marked as picked up!\n";
                } else {
                    cout << "✗ Failed to update order!\n";
                }
                sqlite3_finalize(updateStmt);
            }
        } else {
            cout << "Order not found or not prepared yet!\n";
        }
        sqlite3_finalize(checkStmt);
    }
}
