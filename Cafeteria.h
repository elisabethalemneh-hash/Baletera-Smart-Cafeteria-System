#ifndef CAFETERIA_H
#define CAFETERIA_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>
#include <sqlite3.h>

using namespace std;

// ========== STRUCTURES ==========
struct User {
    int id;
    string studentId;
    string name;
    string phone;
    string password;
    int isAdmin;
    int totalOrders;
    double totalSpent;
};

struct Food {
    int id;
    string name;
    double price;
    int prepTime;
};

struct Order {
    int id;
    int userId;
    int foodId;
    int quantity;
    string orderTime;
    string status;  // waiting, prepared, picked_up
    int day;
    int week;
};

// ========== GLOBAL VARIABLES (extern) ==========
extern sqlite3* db;
extern int currentDay;
extern int currentWeek;
extern const string CAFETERIA_NAME;
extern const string DEFAULT_ADMIN_PASS;
extern User currentUser;

// ========== UTILITY FUNCTION PROTOTYPES ==========
string toUpperStr(string str);
void pressEnter();
void printLine(char ch, int length);
void printHeader(const string& title);
int getInt(const string& prompt);
string getLineInput(const string& prompt);

// ========== DATABASE FUNCTIONS ==========
bool initDatabase();
void createTables();

// ========== VALIDATION & AUTH (Member 2) ==========
bool validateStudentID(const string& id);
bool validateName(const string& name);
bool validatePhone(const string& phone);
bool idExists(const string& studentId);
int findUser(const string& studentId);
bool registerUser();
bool loginUser();
void userChangePassword();

// ========== USER ORDER FUNCTIONS (Member 3) ==========
void viewCafeteriaMenu();
void orderFood();
int getQueueWaitTime(int foodId);
void viewCurrentOrderStatus();
void viewOrderHistory();
void viewOrdersMenu();
void userMenu();
void userPartMenu();

// ========== ADMIN DATA MANAGEMENT (Member 4) ==========
void adminInsertFood();
void adminUpdateFoodPrice();
void adminUpdatePrepTime();
void adminDeleteFood();
void adminViewFoods();
void adminEditMenu();
void adminViewWaitingOrders();
void adminMarkPrepared();
void adminViewPreparedUnpicked();
void adminMarkPickedUp();

// ========== ADMIN REPORTS & CONTROL (Member 5) ==========
void adminViewCustomers();
void generateReport();
void adminReports();
void adminChangeDay();
void adminChangePassword();
void adminMenu();
bool adminLogin();

#endif
