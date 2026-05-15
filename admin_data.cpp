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

// ============ FOOD MANAGEMENT FUNCTIONS ============

void adminViewFoods() {
    const char* sql = "SELECT id, name, price, preparation_time FROM foods WHERE deleted = 0 ORDER BY id";
    sqlite3_stmt* stmt;

    cout << "\n";
    printLine();
    cout << "                    CAFETERIA MENU\n";
    printLine();
    cout << left << setw(5) << "ID"
         << setw(25) << "Food Name"
         << setw(10) << "Price"
         << setw(15) << "Prep Time" << endl;
    printLine();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* name = (const char*)sqlite3_column_text(stmt, 1);
            double price = sqlite3_column_double(stmt, 2);
            int prepTime = sqlite3_column_int(stmt, 3);

            cout << left << setw(5) << id
                 << setw(25) << name
                 << "ETB " << setw(7) << fixed << setprecision(2) << price
                 << setw(15) << prepTime << " mins" << endl;
        }
        sqlite3_finalize(stmt);
    }
    printLine();
}

void adminInsertFood() {
    string name;
    double price;
    int prepTime;

    cout << "\n=== ADD NEW FOOD ITEM ===\n";
    cout << "Enter food name: ";
    cin.ignore();
    getline(cin, name);

    if (name.empty()) {
        cout << "Food name cannot be empty!\n";
        return;
    }

    cout << "Enter price (ETB): ";
    cin >> price;

    if (price <= 0) {
        cout << "Price must be greater than 0!\n";
        return;
    }

    cout << "Enter preparation time (minutes): ";
    cin >> prepTime;

    if (prepTime <= 0) {
        cout << "Preparation time must be greater than 0!\n";
        return;
    }

    const char* sql = "INSERT INTO foods (name, price, preparation_time, deleted) VALUES (?, ?, ?, 0)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_int(stmt, 3, prepTime);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cout << "\n✓ Food item '" << name << "' added successfully!\n";
        } else {
            cout << "✗ Failed to add food item!\n";
        }
        sqlite3_finalize(stmt);
    }
}

void adminUpdateFoodPrice() {
    adminViewFoods();
    int id;
    double newPrice;

    cout << "\n=== UPDATE FOOD PRICE ===\n";
    cout << "Enter food ID to update: ";
    cin >> id;

    const char* checkSql = "SELECT name FROM foods WHERE id = ? AND deleted = 0";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, id);

        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(checkStmt, 0);
            cout << "Current food: " << name << endl;
            cout << "Enter new price (ETB): ";
            cin >> newPrice;

            if (newPrice > 0) {
                const char* updateSql = "UPDATE foods SET price = ? WHERE id = ?";
                sqlite3_stmt* updateStmt;

                if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_double(updateStmt, 1, newPrice);
                    sqlite3_bind_int(updateStmt, 2, id);

                    if (sqlite3_step(updateStmt) == SQLITE_DONE) {
                            cout << "\n✓ Price updated successfully!\n";
                    } else {
                        cout << "✗ Failed to update price!\n";
                    }
                    sqlite3_finalize(updateStmt);
                }
            } else {
                cout << "Price must be greater than 0!\n";
            }
        } else {
            cout << "Food ID not found!\n";
        }
        sqlite3_finalize(checkStmt);
    }
}

void adminUpdatePrepTime() {
    adminViewFoods();
    int id;
    int newPrepTime;

    cout << "\n=== UPDATE PREPARATION TIME ===\n";
    cout << "Enter food ID to update: ";
    cin >> id;

    const char* checkSql = "SELECT name FROM foods WHERE id = ? AND deleted = 0";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, id);

        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(checkStmt, 0);
            cout << "Current food: " << name << endl;
            cout << "Enter new preparation time (minutes): ";
            cin >> newPrepTime;

            if (newPrepTime > 0) {
                const char* updateSql = "UPDATE foods SET preparation_time = ? WHERE id = ?";
                sqlite3_stmt* updateStmt;

                if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int(updateStmt, 1, newPrepTime);
                    sqlite3_bind_int(updateStmt, 2, id);

                    if (sqlite3_step(updateStmt) == SQLITE_DONE) {
                        cout << "\n✓ Preparation time updated successfully!\n";
                    } else {
                        cout << "✗ Failed to update preparation time!\n";
                    }
                    sqlite3_finalize(updateStmt);
                }
            } else {
                cout << "Preparation time must be greater than 0!\n";
            }
        } else {
            cout << "Food ID not found!\n";
        }
        sqlite3_finalize(checkStmt);
    }
}

void adminDeleteFood() {
    adminViewFoods();
    int id;

    cout << "\n=== DELETE FOOD ITEM ===\n";
    cout << "Enter food ID to delete: ";
    cin >> id;

    const char* checkSql = "SELECT name FROM foods WHERE id = ? AND deleted = 0";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, id);

        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(checkStmt, 0);
            cout << "\nAre you sure you want to delete '" << name << "'? (y/n): ";
            char confirm;
            cin >> confirm;

            if (confirm == 'y' || confirm == 'Y') {
                const char* deleteSql = "UPDATE foods SET deleted = 1 WHERE id = ?";
                sqlite3_stmt* deleteStmt;

                if (sqlite3_prepare_v2(db, deleteSql, -1, &deleteStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int(deleteStmt, 1, id);

                    if (sqlite3_step(deleteStmt) == SQLITE_DONE) {
                        cout << "\n✓ Food item deleted successfully!\n";
                    } else {
                        cout << "✗ Failed to delete food item!\n";
                    }
                    sqlite3_finalize(deleteStmt);
                }
            } else {
                cout << "Deletion cancelled.\n";
            }
        } else {
            cout << "Food ID not found!\n";
        }
        sqlite3_finalize(checkStmt);
    }
}

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
