#include "cafeteria.h"
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
        cout << "  No foods available to order.\n";
        pressEnter();
        return;
    }
    printHeader("Order Food");
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

    int numItems = getInt("  How many different food items? (1-10): ", 1, 10);

    Order o;
    o.foodCount     = 0;
    o.totalPrice    = 0;
    o.studentID     = string(user.studentID);
    o.customerName  = string(user.name);
    o.customerPhone = string(user.phone);
    o.prepared      = false;
    o.pickedUp      = false;
    o.day           = currentDay;
    o.week          = currentWeek;
    o.month         = currentMonth;
    int ownPrepTime = 0;

    for (int i = 0; i < numItems; i++) {
        cout << "\n  Item " << (i+1) << ":\n";
        int choice = getInt("  Select food number: ", 1, (int)foods.size());
        int qty    = getInt("  Enter quantity: ", 1, 50);
        o.foods[o.foodCount]            = foods[choice-1].name;
        o.quantities[o.foodCount]       = qty;
        o.preparationTimes[o.foodCount] = foods[choice-1].preparationTime;
        o.totalPrice += foods[choice-1].price * qty;
        if (foods[choice-1].preparationTime > ownPrepTime)
            ownPrepTime = foods[choice-1].preparationTime;
        o.foodCount++;
    }

    int queueWait   = getQueueWaitTime();
    o.estimatedTime = queueWait + ownPrepTime;

    printLine();
    cout << "  ORDER SUMMARY\n";
    printLine();
    for (int i = 0; i < o.foodCount; i++)
        cout << "  - " << o.foods[i] << " x" << o.quantities[i]
             << "  (" << o.preparationTimes[i] << " min)\n";
    cout << "  Total Price     : " << fixed << setprecision(2) << o.totalPrice << " birr\n";
    cout << "  Queue Wait      : " << queueWait << " min\n";
    cout << "  Estimated Pickup: " << o.estimatedTime << " minutes\n";
    printLine();

    string pw = getLineInput("  Confirm your password to place order: ");
    if (pw != string(user.password)) {
        cout << "  Wrong password. Order cancelled.\n";
        pressEnter();
        return;
    }

    orderCounter++;
    saveCounter();
    o.orderNumber = orderCounter;

    vector<Order> orders = loadOrders();
    orders.push_back(o);
    saveOrders(orders);

    cout << "\n  Order placed successfully!\n";
    cout << "  Order Number    : #" << o.orderNumber << "\n";
    cout << "  Estimated Pickup: " << o.estimatedTime << " minutes\n";
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
