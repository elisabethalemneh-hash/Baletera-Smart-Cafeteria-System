#include "cafeteria.h"

// ============================================================
//  GLOBAL VARIABLES DEFINITIONS
// ============================================================
int currentDay   = 1;
int currentWeek  = 1;
int currentMonth = 1;
int orderCounter = 0;

const string CAFETERIA_NAME     = "Central Cafeteria";
const string DEFAULT_ADMIN_PASS = "admin123";

// ============================================================
//  FILE NAMES
// ============================================================
const string FILE_USERS    = "users.txt";
const string FILE_FOODS    = "foods_0.txt";
const string FILE_ORDERS   = "orders_0.txt";
const string FILE_ADMINPW  = "adminpass.txt";
const string FILE_DAY      = "dayinfo.txt";
const string FILE_COUNTER  = "ordercounter.txt";

// ============================================================
//  UTILITY IMPLEMENTATIONS
// ============================================================
string toUpperStr(const string& s) {
    string r = s;
    for (char& c : r) c = (char)toupper((unsigned char)c);
    return r;
}

void pressEnter() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(1000000, '\n');
}

void printLine(char ch, int len) {
    cout << string(len, ch) << "\n";
}

void printHeader(const string& title) {
    cout << "\n";
    printLine('=');
    cout << "  " << CAFETERIA_NAME << "\n";
    cout << "  " << title << "\n";
    printLine('=');
}

int getInt(const string& prompt, int lo, int hi) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        bool ok = true;
        if (s.empty()) ok = false;
        for (char c : s) if (!isdigit((unsigned char)c)) { ok = false; break; }
        if (ok) {
            try {
                long long v = stoll(s);
                if (v >= lo && v <= hi) return (int)v;
            } catch (...) {}
        }
        cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

string getLineInput(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

// ============================================================
//  PERSISTENCE IMPLEMENTATIONS
// ============================================================
void saveDayInfo() {
    ofstream f(FILE_DAY);
    f << currentDay << "\n" << currentWeek << "\n" << currentMonth << "\n";
}

void loadDayInfo() {
    ifstream f(FILE_DAY);
    if (f) f >> currentDay >> currentWeek >> currentMonth;
}

void saveCounter() {
    ofstream f(FILE_COUNTER);
    f << orderCounter << "\n";
}

void loadCounter() {
    ifstream f(FILE_COUNTER);
    if (f) f >> orderCounter;
}

string loadAdminPass() {
    ifstream f(FILE_ADMINPW);
    string pw;
    if (f && getline(f, pw) && !pw.empty()) return pw;
    return DEFAULT_ADMIN_PASS;
}

void saveAdminPass(const string& pw) {
    ofstream f(FILE_ADMINPW);
    f << pw << "\n";
}

// ============================================================
//  USER FILE I/O
// ============================================================
vector<User> loadUsers() {
    vector<User> users;
    ifstream f(FILE_USERS);
    if (!f) return users;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nm, ph, pw;
        if (!getline(ss, id, '|')) continue;
        if (!getline(ss, nm, '|')) continue;
        if (!getline(ss, ph, '|')) continue;
        if (!getline(ss, pw))      continue;
        User u;
        strncpy(u.studentID, id.c_str(), 9);  u.studentID[9]  = '\0';
        strncpy(u.name,      nm.c_str(), 25); u.name[25]      = '\0';
        strncpy(u.phone,     ph.c_str(), 10); u.phone[10]     = '\0';
        u.password = pw;
        users.push_back(u);
    }
    return users;
}

void saveUsers(const vector<User>& users) {
    ofstream f(FILE_USERS);
    for (const auto& u : users)
        f << u.studentID << "|" << u.name << "|" << u.phone << "|" << u.password << "\n";
}

// ============================================================
//  FOOD FILE I/O
// ============================================================
vector<Food> loadFoods() {
    vector<Food> foods;
    ifstream f(FILE_FOODS);
    if (!f) return foods;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string nm, pr, pt;
        if (!getline(ss, nm, '|')) continue;
        if (!getline(ss, pr, '|')) continue;
        if (!getline(ss, pt))      continue;
        Food fd;
        fd.name = nm;
        try { fd.price = stof(pr); fd.preparationTime = stoi(pt); }
        catch (...) { continue; }
        foods.push_back(fd);
    }
    return foods;
}

void saveFoods(const vector<Food>& foods) {
    ofstream f(FILE_FOODS);
    for (const auto& fd : foods)
        f << fd.name << "|" << fixed << setprecision(2) << fd.price
          << "|" << fd.preparationTime << "\n";
}

// ============================================================
//  ORDER FILE I/O
// ============================================================
string serializeOrder(const Order& o) {
    stringstream ss;
    ss << o.orderNumber   << "|"
       << o.studentID     << "|"
       << o.customerName  << "|"
       << o.customerPhone << "|"
       << o.foodCount     << "|";
    for (int i = 0; i < o.foodCount; i++)
        ss << o.foods[i] << "~" << o.quantities[i] << "~" << o.preparationTimes[i] << ";";
    ss << "|"
       << fixed << setprecision(2) << o.totalPrice << "|"
       << (o.prepared ? 1 : 0) << "|"
       << (o.pickedUp ? 1 : 0) << "|"
       << o.estimatedTime << "|"
       << o.day << "|" << o.week << "|" << o.month;
    return ss.str();
}

Order deserializeOrder(const string& line) {
    Order o;
    o.foodCount = 0; o.totalPrice = 0; o.estimatedTime = 0;
    o.prepared = false; o.pickedUp = false;
    o.day = 1; o.week = 1; o.month = 1;
    for (int i = 0; i < 10; i++) { o.quantities[i] = 0; o.preparationTimes[i] = 0; }
    stringstream ss(line);
    string tok;
    getline(ss, tok, '|'); try { o.orderNumber = stoi(tok); } catch (...) { o.orderNumber = 0; }
    getline(ss, o.studentID,     '|');
    getline(ss, o.customerName,  '|');
    getline(ss, o.customerPhone, '|');
    getline(ss, tok, '|'); try { o.foodCount = stoi(tok); } catch (...) {}
    string foodsBlock;
    getline(ss, foodsBlock, '|');
    {
        stringstream fb(foodsBlock);
        string entry;
        int idx = 0;
        while (getline(fb, entry, ';') && idx < 10) {
            if (entry.empty()) continue;
            stringstream es(entry);
            string nm, qt, pt;
            getline(es, nm, '~');
            getline(es, qt, '~');
            getline(es, pt);
            o.foods[idx] = nm;
            try { o.quantities[idx] = stoi(qt); o.preparationTimes[idx] = stoi(pt); }
            catch (...) {}
            idx++;
        }
    }
    getline(ss, tok, '|'); try { o.totalPrice    = stof(tok); } catch (...) {}
    getline(ss, tok, '|'); o.prepared = (tok == "1");
    getline(ss, tok, '|'); o.pickedUp = (tok == "1");
    getline(ss, tok, '|'); try { o.estimatedTime = stoi(tok); } catch (...) {}
    getline(ss, tok, '|'); try { o.day           = stoi(tok); } catch (...) {}
    getline(ss, tok, '|'); try { o.week          = stoi(tok); } catch (...) {}
    getline(ss, tok);      try { o.month         = stoi(tok); } catch (...) {}
    return o;
}

vector<Order> loadOrders() {
    vector<Order> orders;
    ifstream f(FILE_ORDERS);
    if (!f) return orders;
    string line;
    while (getline(f, line))
        if (!line.empty()) orders.push_back(deserializeOrder(line));
    return orders;
}

void saveOrders(const vector<Order>& orders) {
    ofstream f(FILE_ORDERS);
    for (const auto& o : orders)
        f << serializeOrder(o) << "\n";
}

int getQueueWaitTime() {
    vector<Order> orders = loadOrders();
    int total = 0;
    for (const auto& o : orders)
        if (!o.prepared) total += o.estimatedTime;
    return total;
}

int findOrderIndex(vector<Order>& orders, int num) {
    for (int i = 0; i < (int)orders.size(); i++)
        if (orders[i].orderNumber == num) return i;
    return -1;
}

// ============================================================
//  MAIN FUNCTION
// ============================================================
int main() {
    loadDayInfo();
    loadCounter();

    while (true) {
        printHeader("Main Menu");
        cout << "  1. User\n";
        cout << "  2. Admin\n";
        cout << "  3. Exit\n";
        int ch = getInt("  Choice: ", 1, 3);
        if      (ch == 1) userPartMenu();
        else if (ch == 2) adminLogin();
        else {
            cout << "\n  Thank you for using " << CAFETERIA_NAME << ". Goodbye!\n\n";
            return 0;
        }
    }
}
