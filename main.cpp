#include "classes/databaseManager.hpp"
using namespace std;

// Thrown when the input stream ends (Ctrl+D / Ctrl+Z) so the program can exit cleanly
struct InputEnded {};

// Thrown when login fails after logout, so the program can exit
struct LoginFailed {};

// ---------- input helpers ----------
string ask(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) throw InputEnded();
    return s;
}

// Keeps asking until the user enters a whole number between lo and hi
int askInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = trim(ask(prompt));
        if (isInteger(s)) {
            long long v = stoll(s);
            if (v >= lo && v <= hi) return (int)v;
        }
        cout << "Please enter a number from " << lo << " to " << hi << ".\n";
    }
}

bool askYesNo(const string& prompt) {
    while (true) {
        string s = toLower(trim(ask(prompt + " (y/n): ")));
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        cout << "Please answer y or n.\n";
    }
}

ColType askType() {
    int t = askInt("Type (1 = INT, 2 = DOUBLE, 3 = STRING): ", 1, 3);
    if (t == 1) return INT;
    if (t == 2) return DOUBLE;
    return STRING;
}

// ---------- login ----------
// Up to 3 attempts. Returns false if all fail
bool loginScreen(DbManager& mgr) {
    for (int attempt = 1; attempt <= 3; attempt++) {
        string user = ask("Username: ");
        string pass = ask("Password: ");
        if (mgr.login(user, pass)) {
            cout << "Welcome, " << mgr.whoAmI() << "\n";
            return true;
        }
        cout << "Wrong username or password (" << attempt << "/3)\n";
    }
    return false;
}

// ---------- actions: tables ----------
void showTables(DbManager& mgr) {
    cout << "\n" << mgr.showTables();
}

void describeTable(DbManager& mgr) {
    string t = ask("Table name: ");
    cout << "\n" << mgr.describeTable(t);
}

void createTable(DbManager& mgr) {
    string name = ask("Table name: ");
    int count = askInt("Number of columns: ", 1, 50);

    vector<Col> cols;
    for (int i = 1; i <= count; i++) {
        cout << "\nColumn " << i << " of " << count << "\n";
        string cname = ask("  Name: ");
        ColType type = askType();
        bool pk = askYesNo("  Primary key?");
        bool nn = pk ? true : askYesNo("  Not null?");
        cols.push_back(Col(cname, type, pk, nn));
    }
    mgr.createTable(name, cols);
    cout << "Table " << name << " created.\n";
}

void dropTable(DbManager& mgr) {
    string name = ask("Table name: ");
    if (!askYesNo("Delete table " + name + " and all its rows?")) {
        cout << "Cancelled.\n";
        return;
    }
    mgr.dropTable(name);
    cout << "Table " << name << " dropped.\n";
}

void addColumn(DbManager& mgr) {
    string table = ask("Table name: ");
    string cname = ask("New column name: ");
    ColType type = askType();
    bool nn = askYesNo("Not null?");
    string def = ask("Default value for existing rows (Enter for empty): ");
    mgr.addColumn(table, Col(cname, type, false, nn), def);
    cout << "Column added.\n";
}

void dropColumn(DbManager& mgr) {
    string table = ask("Table name: ");
    string cname = ask("Column to drop: ");
    if (!askYesNo("Drop column " + cname + "?")) {
        cout << "Cancelled.\n";
        return;
    }
    mgr.dropColumn(table, cname);
    cout << "Column dropped.\n";
}

// ---------- actions: data ----------
void insertRow(DbManager& mgr) {
    string table = ask("Table name: ");
    vector<Col> cols = mgr.getColumns(table);

    cout << "Enter a value for each column (Enter for empty):\n";
    vector<string> values;
    for (const Col& c : cols) values.push_back(ask("  " + c.describe() + ": "));

    mgr.insert(table, values);
    cout << "1 row inserted.\n";
}

void selectRows(DbManager& mgr) {
    string table = ask("Table name: ");
    int mode = askInt("  1. All rows\n  2. Rows matching a value\nChoose: ", 1, 2);

    if (mode == 1) {
        cout << "\n" << mgr.select(table);
    } else {
        string col = ask("Column: ");
        string val = ask("Value: ");
        cout << "\n" << mgr.select(table, col, val);
    }
}

void updateRows(DbManager& mgr) {
    string table = ask("Table name: ");
    string whereCol = ask("Find rows where column: ");
    string whereVal = ask("equals value: ");
    string setCol = ask("Change column: ");
    string newVal = ask("To new value: ");
    int n = mgr.update(table, whereCol, whereVal, setCol, newVal);
    cout << n << " row(s) updated.\n";
}

void deleteRows(DbManager& mgr) {
    string table = ask("Table name: ");
    string col = ask("Delete rows where column: ");
    string val = ask("equals value: ");
    if (!askYesNo("Delete all matching rows?")) {
        cout << "Cancelled.\n";
        return;
    }
    int n = mgr.deleteWhere(table, col, val);
    cout << n << " row(s) deleted.\n";
}

// ---------- actions: account ----------
void changePassword(DbManager& mgr) {
    string oldPw = ask("Current password: ");
    string newPw = ask("New password: ");
    mgr.changeOwnPassword(oldPw, newPw);
    cout << "Password changed.\n";
}

void showPermissions(DbManager& mgr) {
    auto yn = [](bool b) { return b ? "yes" : "no"; };
    cout << "\nYour permissions (" << mgr.whoAmI() << "):\n"
         << "  View data ............ " << yn(mgr.canSelect()) << "\n"
         << "  Insert rows .......... " << yn(mgr.canInsert()) << "\n"
         << "  Update rows .......... " << yn(mgr.canUpdate()) << "\n"
         << "  Delete rows .......... " << yn(mgr.canDelete()) << "\n"
         << "  Create tables ........ " << yn(mgr.canCreateTable()) << "\n"
         << "  Drop tables .......... " << yn(mgr.canDropTable()) << "\n"
         << "  Alter tables ......... " << yn(mgr.canAlterTable()) << "\n"
         << "  Manage users ......... " << yn(mgr.canManageUsers()) << "\n";
}

// ---------- user management (admin only) ----------
void userMenu(DbManager& mgr) {
    while (true) {
        cout << "\n--- User Management ---\n"
             << "  1. List users\n"
             << "  2. Add a user\n"
             << "  3. Remove a user\n"
             << "  4. Reset a user's password\n"
             << "  0. Back\n\n";
        int choice = askInt("Choose an option: ", 0, 4);
        if (choice == 0) return;

        try {
            if (choice == 1) {
                cout << "\nUsers:\n";
                for (const string& u : mgr.listUsers()) cout << "  " << u << "\n";
            }
            else if (choice == 2) {
                string name = ask("Username: ");
                string pass = ask("Password: ");
                int r = askInt("Role (1 = ADMIN, 2 = REGULAR): ", 1, 2);
                mgr.addUser(name, pass, r == 1 ? "ADMIN" : "REGULAR");
                cout << "User " << name << " added.\n";
            }
            else if (choice == 3) {
                string name = ask("Username to remove: ");
                if (!askYesNo("Remove user " + name + "?")) {
                    cout << "Cancelled.\n";
                    continue;
                }
                mgr.removeUser(name);
                cout << "User " << name << " removed.\n";
            }
            else if (choice == 4) {
                string name = ask("Username: ");
                string pass = ask("New password: ");
                mgr.resetPassword(name, pass);
                cout << "Password reset.\n";
            }
        } catch (const InputEnded&) {
            throw;
        } catch (const exception& e) {
            cout << "Error: " << e.what() << "\n";
        }
    }
}

// ---------- role-based main menu ----------
struct MenuItem {
    string section;
    string label;
    bool allowed;           // false = hidden for this user
    function<void()> action;
};

// Lists every possible option with the permission it needs
vector<MenuItem> buildMenu(DbManager& mgr) {
    return {
        {"Tables", "Show all tables",    mgr.canSelect(),      [&] { showTables(mgr); }},
        {"Tables", "Describe a table",   mgr.canSelect(),      [&] { describeTable(mgr); }},
        {"Tables", "Create a table",     mgr.canCreateTable(), [&] { createTable(mgr); }},
        {"Tables", "Drop a table",       mgr.canDropTable(),   [&] { dropTable(mgr); }},
        {"Tables", "Add a column",       mgr.canAlterTable(),  [&] { addColumn(mgr); }},
        {"Tables", "Drop a column",      mgr.canAlterTable(),  [&] { dropColumn(mgr); }},

        {"Data", "Insert a row",         mgr.canInsert(),      [&] { insertRow(mgr); }},
        {"Data", "Select rows",          mgr.canSelect(),      [&] { selectRows(mgr); }},
        {"Data", "Update rows",          mgr.canUpdate(),      [&] { updateRows(mgr); }},
        {"Data", "Delete rows",          mgr.canDelete(),      [&] { deleteRows(mgr); }},

        {"Administration", "User management", mgr.canManageUsers(), [&] { userMenu(mgr); }},

        {"Account", "My permissions",    true, [&] { showPermissions(mgr); }},
        {"Account", "Change my password", true, [&] { changePassword(mgr); }},
        {"Account", "Logout",            true, [&] {
            mgr.logout();
            cout << "Logged out.\n\n";
            if (!loginScreen(mgr)) throw LoginFailed();
        }},
    };
}

int main() {
    try {
        DbManager mgr("mydb", "database.db", "users.db");

        cout << "=== Mini Database ===\n";
        cout << "First run? Log in with admin / admin123 and change the password.\n\n";

        if (!loginScreen(mgr)) {
            cout << "Login failed. Exiting.\n";
            return 1;
        }

        while (true) {
            // Rebuild the menu every time, so it always matches the current user
            vector<MenuItem> visible;
            for (MenuItem& item : buildMenu(mgr))
                if (item.allowed) visible.push_back(item);

            cout << "\n===== MINI DATABASE =====\n"
                 << "Logged in as: " << mgr.whoAmI() << "\n";
            string lastSection;
            for (size_t i = 0; i < visible.size(); i++) {
                if (visible[i].section != lastSection) {
                    cout << "\n  --- " << visible[i].section << " ---\n";
                    lastSection = visible[i].section;
                }
                cout << "  " << setw(2) << (i + 1) << ". " << visible[i].label << "\n";
            }
            cout << "\n   0. Exit\n\n";

            int choice = askInt("Choose an option: ", 0, (int)visible.size());
            if (choice == 0) break;

            try {
                visible[choice - 1].action();
            } catch (const InputEnded&) {
                throw;
            } catch (const LoginFailed&) {
                throw;
            } catch (const exception& e) {
                cout << "Error: " << e.what() << "\n";
            }
        }

        mgr.save();
        cout << "Goodbye.\n";
    } catch (const LoginFailed&) {
        cout << "Login failed. Exiting.\n";
        return 1;
    } catch (const InputEnded&) {
        cout << "\nInput closed. Exiting.\n";
    } catch (const exception& e) {
        cout << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}