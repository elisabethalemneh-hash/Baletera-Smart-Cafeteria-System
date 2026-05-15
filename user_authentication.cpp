#include "cafeteria.h"

// ============================================================
//  VALIDATION FUNCTIONS
// ============================================================

bool validateStudentID(const string& id) {
    if (id.size() != 9) return false;
    if (toUpperStr(id.substr(0, 3)) != "ETS") return false;
    for (int i = 3; i < 9; i++)
        if (!isdigit((unsigned char)id[i])) return false;
    return true;
}

bool validateName(const string& name) {
    if (name.empty() || name.size() > 25) return false;
    for (char c : name)
        if (!isalpha((unsigned char)c) && c != ' ') return false;
    return true;
}

bool validatePhone(const string& phone) {
    if (phone.size() != 10) return false;
    for (char c : phone)
        if (!isdigit((unsigned char)c)) return false;
    return (phone.substr(0, 2) == "07" || phone.substr(0, 2) == "09");
}

bool idExists(const string& id) {
    for (const auto& u : loadUsers())
        if (toUpperStr(string(u.studentID)) == toUpperStr(id)) return true;
    return false;
}

int findUser(const vector<User>& users, const string& id) {
    for (int i = 0; i < (int)users.size(); i++)
        if (toUpperStr(string(users[i].studentID)) == toUpperStr(id)) return i;
    return -1;
}

// ============================================================
//  USER REGISTRATION
// ============================================================

void registerUser() {
    printHeader("New User Registration");

    string id;
    while (true) {
        id = getLineInput("  Enter Student ID (e.g. ETS123456): ");
        if (!validateStudentID(id)) {
            cout << "  Invalid ID. Must start with ETS followed by exactly 6 digits.\n";
            continue;
        }
        if (idExists(id)) {
            cout << "  This Student ID is already registered.\n";
            continue;
        }
        break;
    }

    string name;
    while (true) {
        name = getLineInput("  Enter Full Name (max 25 chars, letters & spaces): ");
        if (!validateName(name)) {
            cout << "  Invalid name. Use letters and spaces only, max 25 characters.\n";
            continue;
        }
        break;
    }

    string phone;
    while (true) {
        phone = getLineInput("  Enter Phone Number (10 digits, starts with 07 or 09): ");
        if (!validatePhone(phone)) {
            cout << "  Invalid phone. Must be 10 digits starting with 07 or 09.\n";
            continue;
        }
        break;
    }

    cout << "  Tip: Use special characters to make your password stronger.\n";
    string pw, pw2;
    while (true) {
        pw  = getLineInput("  Create Password: ");
        pw2 = getLineInput("  Confirm Password: ");
        if (pw.empty()) { cout << "  Password cannot be empty.\n"; continue; }
        if (pw != pw2)  { cout << "  Passwords do not match. Try again.\n"; continue; }
        break;
    }

    User u;
    strncpy(u.studentID, toUpperStr(id).c_str(), 9); u.studentID[9] = '\0';
    strncpy(u.name,      name.c_str(), 25);           u.name[25]     = '\0';
    strncpy(u.phone,     phone.c_str(), 10);          u.phone[10]    = '\0';
    u.password = pw;

    vector<User> users = loadUsers();
    users.push_back(u);
    saveUsers(users);

    cout << "\n  Registration successful! Welcome, " << u.name << "!\n";
    pressEnter();
}

// ============================================================
//  USER LOGIN
// ============================================================

int loginUser(vector<User>& users) {
    printHeader("User Login");

    string id = getLineInput("  Enter Student ID: ");
    int idx = findUser(users, id);
    if (idx == -1) {
        cout << "  Student ID not found.\n";
        pressEnter();
        return -1;
    }

    const int MAX_ATTEMPTS = 5;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        string pw = getLineInput("  Enter Password: ");
        if (pw == users[idx].password) {
            cout << "\n  Login successful! Welcome, " << users[idx].name << "!\n";
            pressEnter();
            return idx;
        }
        int left = MAX_ATTEMPTS - attempt;
        if (left > 0)
            cout << "  Wrong password. You have " << left << " attempt(s) left.\n";
    }
    cout << "  Your attempt has ended. Try again later.\n";
    pressEnter();
    return -1;
}

// ============================================================
//  USER CHANGE PASSWORD
// ============================================================

void userChangePassword(User& user, vector<User>& users, int idx) {
    printHeader("Change Password");
    string oldPw = getLineInput("  Enter current password: ");
    if (oldPw != user.password) {
        cout << "  Incorrect password.\n";
        pressEnter();
        return;
    }
    string newPw, confirm;
    while (true) {
        newPw   = getLineInput("  Enter new password: ");
        confirm = getLineInput("  Confirm new password: ");
        if (newPw.empty())    { cout << "  Password cannot be empty.\n"; continue; }
        if (newPw != confirm) { cout << "  Passwords do not match.\n";   continue; }
        break;
    }
    user.password       = newPw;
    users[idx].password = newPw;
    saveUsers(users);
    cout << "  Password changed successfully.\n";
    pressEnter();
}
