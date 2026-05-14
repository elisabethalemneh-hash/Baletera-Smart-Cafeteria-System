#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include <iostream>
#include <vector>
#include <string>
using namespace std;

struct Order {
    int orderNum;
    string studentID;
    string studentName;
    string studentPhone;
    string cafeteria;
    string foodNames[5];
    int quantities[5];
    int prepTimes[5];
    int foodCount;
    float totalPrice;
    string status;
    int waitTime;
    int day;
    int week;
    int month;
    string deliveryLocation;
};

int currentDay = 1;
int currentWeek = 1;
int currentMonth = 1;
int orderCounter = 0;

void pause() {
    cout << "\n(Press Enter to continue...)";
    cin.ignore();
    cin.get();
}

int getIntInput(string prompt, int minVal, int maxVal) {
    int val;
    while(true) {
        cout << prompt;
        cin >> val;
        if(cin.fail() || val < minVal || val > maxVal) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Oops, please enter a number between " << minVal << " and " << maxVal << ".\n";
        } else {
            cin.ignore();
            return val;
        }
    }
}

string getStringInput(string prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

vector<Order> loadOrders(int cafIndex) {
    vector<Order> orders;
    return orders;
}

void saveOrders(int cafIndex, vector<Order>& orders) {}

string getAdminPassword(int cafIndex) {
    return "green123";
}

void saveAdminPassword(int cafIndex, string pass) {}

void saveOrderCounter() {}
void saveDayInfo() {}

const int NUM_CAFETERIAS = 5;
const string CAFETERIA_NAMES[] = {"Green KK", "Yellow KK", "G-One", "Teachers", "Central"};

// Report Functions
// Written by: Hana Berhe (ETS0690/17)

void adminReport(int cafIndex) {
    cout << "\n--- Report for " << CAFETERIA_NAMES[cafIndex] << " ---\n";

    vector<Order> orders = loadOrders(cafIndex);

    int totalOrders = 0;
    float totalRevenue = 0;
    int delivered = 0;
    int picked = 0;

    string foodNames[100];
    int foodQuantities[100];
    int foodCount = 0;

    for(int i = 0; i < orders.size(); i++) {
        Order& o = orders[i];
        totalOrders++;
        totalRevenue += o.totalPrice;
        if(o.deliveryLocation != "counter") delivered++;
        else picked++;

        for(int j = 0; j < o.foodCount; j++) {
            string fname = o.foodNames[j];
            int qty = o.quantities[j];
            bool found = false;
            for(int k = 0; k < foodCount; k++) {
                if(foodNames[k] == fname) {
                    foodQuantities[k] += qty;
                    found = true;
                    break;
                }
            }
            if(!found && foodCount < 100) {
                foodNames[foodCount] = fname;
                foodQuantities[foodCount] = qty;
                foodCount++;
            }
        }
    }

    cout << "Total Orders: " << totalOrders << endl;
    cout << "Total Revenue: " << totalRevenue << " birr" << endl;
    cout << "Pickup Orders: " << picked << endl;
    cout << "Delivery Orders: " << delivered << endl;

    if(foodCount > 0) {
        cout << "\nFoods Sold:\n";
        for(int i = 0; i < foodCount && i < 20; i++) {
            cout << "  " << foodNames[i] << " - " << foodQuantities[i] << " units\n";
        }
    }

    cout << "\nDay " << currentDay << ", Week " << currentWeek << ", Month " << currentMonth << endl;
    pause();
}

void adminChangeDay() {
    cout << "\n--- Change Day ---\n";
    cout << "Current: Day " << currentDay << ", Week " << currentWeek << ", Month " << currentMonth << endl;
    int choice = getIntInput("Advance to next day? (1 = yes, 2 = no): ", 1, 2);

    if(choice == 1) {
        currentDay++;
        orderCounter = 0;
        saveOrderCounter();
        if(currentDay % 7 == 1) currentWeek++;
        if(currentDay % 30 == 1) currentMonth++;
        saveDayInfo();
        cout << "Moved forward to Day " << currentDay << ", Week " << currentWeek << ", Month " << currentMonth << endl;
    }
    pause();
}

void adminMenu(int cafIndex) {
    while(true) {
        cout << "\nAdmin Menu - " << CAFETERIA_NAMES[cafIndex] << endl;
        cout << "Day " << currentDay << " | Week " << currentWeek << " | Month " << currentMonth << endl;
        cout << "1. Edit Menu\n";
        cout << "2. View Waiting Orders\n";
        cout << "3. Mark Order as Ready\n";
        cout << "4. View Reports\n";
        cout << "5. Change Day\n";
        cout << "6. Change Admin Password\n";
        cout << "7. Logout\n";

        int choice = getIntInput("Choice: ", 1, 7);

        if(choice == 4) adminReport(cafIndex);
        else if(choice == 5) adminChangeDay();
        else if(choice == 7) {
            cout << "Logged out.\n";
            pause();
            return;
        }
        else {
            cout << "Feature coming soon...\n";
            pause();
        }
    }
}

void adminSection() {
    while(true) {
        cout << "\nAdmin Login - Select Cafeteria\n";
        for(int i = 0; i < NUM_CAFETERIAS; i++) {
            cout << i+1 << ". " << CAFETERIA_NAMES[i] << endl;
        }
        cout << (NUM_CAFETERIAS+1) << ". Back\n";

        int choice = getIntInput("Choice: ", 1, NUM_CAFETERIAS+1);
        if(choice == NUM_CAFETERIAS+1) return;

        int cafIndex = choice - 1;
        string correctPass = getAdminPassword(cafIndex);

        for(int attempt = 1; attempt <= 3; attempt++) {
            string entered = getStringInput("Enter admin password: ");
            if(entered == correctPass) {
                adminMenu(cafIndex);
                break;
            }
            int left = 3 - attempt;
            if(left > 0) cout << "Wrong password. " << left << " attempts left.\n";
            else cout << "Access denied.\n";
        }
    }
}

int main() {
    cout << "\nWelcome to the Baletera Report System\n";
    cout << "Developed by Hana Berhe (ETS0690/17)\n";
    adminSection();
    return 0;
}
