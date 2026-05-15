#ifndef CAFETERIA_H
#define CAFETERIA_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstring>

using namespace std;

// ============================================================
//  STRUCTURES
// ============================================================
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

// ============================================================
//  GLOBAL VARIABLES 
// ============================================================
extern int currentDay;
extern int currentWeek;
extern int currentMonth;
extern int orderCounter;
extern const string CAFETERIA_NAME;
extern const string DEFAULT_ADMIN_PASS;

// ============================================================
//  FILE NAMES 
// ============================================================
extern const string FILE_USERS;
extern const string FILE_FOODS;
extern const string FILE_ORDERS;
extern const string FILE_ADMINPW;
extern const string FILE_DAY;
extern const string FILE_COUNTER;

// ============================================================
//  UTILITY FUNCTION PROTOTYPES
// ============================================================
string toUpperStr(const string& s);
void pressEnter();
void printLine(char ch = '-', int len = 60);
void printHeader(const string& title);
int getInt(const string& prompt, int lo, int hi);
string getLineInput(const string& prompt);

// ============================================================
//  PERSISTENCE FUNCTIONS 
// ============================================================
void saveDayInfo();
void loadDayInfo();
void saveCounter();
void loadCounter();
string loadAdminPass();
void saveAdminPass(const string& pw);

// ============================================================
//  FILE I/O FUNCTIONS 
// ============================================================
vector<User> loadUsers();
void saveUsers(const vector<User>& users);
vector<Food> loadFoods();
void saveFoods(const vector<Food>& foods);
string serializeOrder(const Order& o);
Order deserializeOrder(const string& line);
vector<Order> loadOrders();
void saveOrders(const vector<Order>& orders);
int getQueueWaitTime();
int findOrderIndex(vector<Order>& orders, int num);

// ============================================================
//  VALIDATION FUNCTIONS
// ============================================================
bool validateStudentID(const string& id);
bool validateName(const string& name);
bool validatePhone(const string& phone);
bool idExists(const string& id);
int findUser(const vector<User>& users, const string& id);

// ============================================================
//  USER AUTHENTICATION FUNCTIONS
// ============================================================
void registerUser();
int loginUser(vector<User>& users);
void userChangePassword(User& user, vector<User>& users, int idx);

// ============================================================
//  USER ORDERING FUNCTIONS 
// ============================================================
bool viewCafeteriaMenu();
void orderFood(const User& user);
void viewCurrentOrderStatus(const string& studentID);
void viewOrderHistory(const string& studentID);
void viewOrdersMenu(const string& studentID);
void userMenu(int userIdx);
void userPartMenu();

// ============================================================
//  ADMIN FUNCTIONS 
// ============================================================
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
void adminViewCustomers();
void generateReport(const string& label, int filterDay, int filterWeek, int filterMonth);
void adminReports();
void adminChangeDay();
void adminChangePassword();
void adminMenu();
void adminLogin();

#endif
