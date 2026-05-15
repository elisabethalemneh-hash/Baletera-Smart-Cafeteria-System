#include <iostream>
#include <string>
#include <fstream> 
#include <iomainp>
void adminInsertFood() {
    printHeader("Insert New Food");
    string name = getLineInput("  Food Name: ");
    if (name.empty()) { cout << "  Name cannot be empty.\n"; pressEnter(); return; }
    float price = 0;
    while (true) {
        string s = getLineInput("  Price (birr): ");
        try { price = stof(s); if (price > 0) break; } catch (...) {}
        cout << "  Enter a valid positive price.\n";
    }
    int prepTime = getInt("  Preparation Time (minutes): ", 1, 300);

    vector<Food> foods = loadFoods();
    // check duplicate
    for (const auto& f : foods)
        if (toUpperStr(f.name) == toUpperStr(name)) {
            cout << "  A food with that name already exists.\n";
            pressEnter();
            return;
        }
    Food fd; fd.name = name; fd.price = price; fd.preparationTime = prepTime;
    foods.push_back(fd);
    saveFoods(foods);
    cout << "  Food added successfully.\n";
    pressEnter();
}

void adminUpdateFoodPrice() {
    printHeader("Update Food Price");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
    string name = getLineInput("  Enter food name to update: ");
    bool found = false;
    for (auto& f : foods) {
        if (toUpperStr(f.name) == toUpperStr(name)) {
            float price = 0;
            while (true) {
                string s = getLineInput("  New Price (birr): ");
                try { price = stof(s); if (price > 0) break; } catch (...) {}
                cout << "  Enter a valid positive price.\n";
            }
            f.price = price;
            saveFoods(foods);
            cout << "  Price updated successfully.\n";
            found = true;
            break;
        }
    }
    if (!found) cout << "  Food not found.\n";
    pressEnter();
}

void adminUpdatePrepTime() {
    printHeader("Update Preparation Time");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
    string name = getLineInput("  Enter food name to update: ");
    bool found = false;
    for (auto& f : foods) {
        if (toUpperStr(f.name) == toUpperStr(name)) {
            int pt = getInt("  New Preparation Time (minutes): ", 1, 300);
            f.preparationTime = pt;
            saveFoods(foods);
            cout << "  Preparation time updated successfully.\n";
            found = true;
            break;
        }
    }
    if (!found) cout << "  Food not found.\n";
    pressEnter();
}

void adminDeleteFood() {
    printHeader("Delete Food");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
    string name = getLineInput("  Enter food name to delete: ");
    int before = (int)foods.size();
    foods.erase(remove_if(foods.begin(), foods.end(),
        [&](const Food& f){ return toUpperStr(f.name) == toUpperStr(name); }),
        foods.end());
    if ((int)foods.size() < before) {
        saveFoods(foods);
        cout << "  Food deleted successfully.\n";
    } else {
        cout << "  Food not found.\n";
    }
    pressEnter();
}

void adminViewFoods() {
    printHeader("Food List");
    vector<Food> foods = loadFoods();
    if (foods.empty()) { cout << "  No foods available.\n"; pressEnter(); return; }
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
    pressEnter();
}

void adminEditMenu() {
    while (true) {
        printHeader("Edit Menu");
        cout << "  1. Insert New Food\n";
        cout << "  2. Update Food Price\n";
        cout << "  3. Update Preparation Time\n";
        cout << "  4. Delete Food\n";
        cout << "  5. View Foods\n";
        cout << "  6. Back\n";
        int ch = getInt("  Choice: ", 1, 6);
        if      (ch == 1) adminInsertFood();
        else if (ch == 2) adminUpdateFoodPrice();
        else if (ch == 3) adminUpdatePrepTime();
        else if (ch == 4) adminDeleteFood();
        else if (ch == 5) adminViewFoods();
        else return;
    }
}