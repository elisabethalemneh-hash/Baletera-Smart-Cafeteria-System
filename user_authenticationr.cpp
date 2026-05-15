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
