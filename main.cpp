#include "classes/databaseManager.hpp"
using namespace std;

// Splits a line into words. Text inside "double quotes" stays together,
// and "" gives an empty value
vector<string> tokenize(const string& line) {
    vector<string> tokens;
    string cur;
    bool inQuotes = false, inToken = false;
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
            inToken = true;
        } else if (isspace((unsigned char)c) && !inQuotes) {
            if (inToken) { tokens.push_back(cur); cur.clear(); inToken = false; }
        } else {
            cur += c;
            inToken = true;
        }
    }
    if (inQuotes) throw invalid_argument("Missing closing quote");
    if (inToken) tokens.push_back(cur);
    return tokens;
}

// Turns "name:TYPE:pk:nn" into a Col. Flags are optional: pk = primary key, nn = not null
Col parseCol(const string& spec) {
    vector<string> p = split(spec, ':');
    if (p.size() < 2)
        throw invalid_argument("Column format is name:TYPE[:pk][:nn], got: " + spec);
    bool pk = false, nn = false;
    for (size_t i = 2; i < p.size(); i++) {
        string f = toLower(trim(p[i]));
        if (f == "pk") pk = true;
        else if (f == "nn") nn = true;
        else throw invalid_argument("Unknown column flag: " + p[i]);
    }
    return Col(p[0], Col::typeFromString(p[1]), pk, nn);
}

void printHelp() {
    cout << "Commands (use \"double quotes\" for values with spaces):\n"
         << "  tables                                    list all tables\n"
         << "  describe <table>                          show columns of a table\n"
         << "  create <table> <col:TYPE[:pk][:nn]> ...   create a table\n"
         << "  drop <table>                              delete a table\n"
         << "  addcol <table> <col:TYPE[:nn]> [default]  add a column\n"
         << "  dropcol <table> <col>                     remove a column\n"
         << "  insert <table> <v1> <v2> ...              add a row\n"
         << "  select <table> [col value]                show all rows, or matching rows\n"
         << "  update <table> <whereCol> <whereValue> <setCol> <newValue>\n"
         << "  delete <table> <col> <value>              delete matching rows\n"
         << "  users                                     list users (admin)\n"
         << "  adduser <name> <password> <ADMIN|REGULAR> add a user (admin)\n"
         << "  rmuser <name>                             remove a user (admin)\n"
         << "  resetpw <name> <newPassword>              reset someone's password (admin)\n"
         << "  passwd <oldPassword> <newPassword>        change your own password\n"
         << "  whoami | logout | help | exit\n"
         << "Types: INT, DOUBLE, STRING\n"
         << "Example: create students id:INT:pk name:STRING:nn marks:DOUBLE\n";
}

// Runs one command. Returns false when the program should exit
bool runCommand(DbManager& mgr, const vector<string>& t) {
    string cmd = toLower(t[0]);
    size_t n = t.size();

    auto need = [&](size_t count, const string& usage) {
        if (n < count) throw invalid_argument("Usage: " + usage);
    };

    if (cmd == "help") printHelp();
    else if (cmd == "exit" || cmd == "quit") return false;
    else if (cmd == "whoami") cout << mgr.whoAmI() << "\n";
    else if (cmd == "logout") { mgr.logout(); cout << "Logged out.\n"; }

    else if (cmd == "tables") cout << mgr.showTables();
    else if (cmd == "describe") {
        need(2, "describe <table>");
        cout << mgr.describeTable(t[1]);
    }

    else if (cmd == "create") {
        need(3, "create <table> <col:TYPE[:pk][:nn]> ...");
        vector<Col> cols;
        for (size_t i = 2; i < n; i++) cols.push_back(parseCol(t[i]));
        mgr.createTable(t[1], cols);
        cout << "Table " << t[1] << " created.\n";
    }
    else if (cmd == "drop") {
        need(2, "drop <table>");
        mgr.dropTable(t[1]);
        cout << "Table " << t[1] << " dropped.\n";
    }
    else if (cmd == "addcol") {
        need(3, "addcol <table> <col:TYPE[:nn]> [default]");
        mgr.addColumn(t[1], parseCol(t[2]), n > 3 ? t[3] : "");
        cout << "Column added.\n";
    }
    else if (cmd == "dropcol") {
        need(3, "dropcol <table> <col>");
        mgr.dropColumn(t[1], t[2]);
        cout << "Column dropped.\n";
    }

    else if (cmd == "insert") {
        need(3, "insert <table> <v1> <v2> ...");
        mgr.insert(t[1], vector<string>(t.begin() + 2, t.end()));
        cout << "1 row inserted.\n";
    }
    else if (cmd == "select") {
        need(2, "select <table> [col value]");
        if (n >= 4) cout << mgr.select(t[1], t[2], t[3]);
        else cout << mgr.select(t[1]);
    }
    else if (cmd == "update") {
        need(6, "update <table> <whereCol> <whereValue> <setCol> <newValue>");
        int c = mgr.update(t[1], t[2], t[3], t[4], t[5]);
        cout << c << " row(s) updated.\n";
    }
    else if (cmd == "delete") {
        need(4, "delete <table> <col> <value>");
        int c = mgr.deleteWhere(t[1], t[2], t[3]);
        cout << c << " row(s) deleted.\n";
    }

    else if (cmd == "users") {
        for (const string& u : mgr.listUsers()) cout << "  " << u << "\n";
    }
    else if (cmd == "adduser") {
        need(4, "adduser <name> <password> <ADMIN|REGULAR>");
        mgr.addUser(t[1], t[2], t[3]);
        cout << "User " << t[1] << " added.\n";
    }
    else if (cmd == "rmuser") {
        need(2, "rmuser <name>");
        mgr.removeUser(t[1]);
        cout << "User " << t[1] << " removed.\n";
    }
    else if (cmd == "resetpw") {
        need(3, "resetpw <name> <newPassword>");
        mgr.resetPassword(t[1], t[2]);
        cout << "Password reset.\n";
    }
    else if (cmd == "passwd") {
        need(3, "passwd <oldPassword> <newPassword>");
        mgr.changeOwnPassword(t[1], t[2]);
        cout << "Password changed.\n";
    }

    else cout << "Unknown command: " << t[0] << " (type help)\n";
    return true;
}

// Asks for credentials, up to 3 attempts. Returns false if all fail or input ends
bool loginScreen(DbManager& mgr) {
    for (int attempt = 1; attempt <= 3; attempt++) {
        string user, pass;
        cout << "Username: ";
        if (!getline(cin, user)) return false;
        cout << "Password: ";
        if (!getline(cin, pass)) return false;

        if (mgr.login(user, pass)) {
            cout << "Welcome, " << mgr.whoAmI() << "\n";
            return true;
        }
        cout << "Wrong username or password (" << attempt << "/3)\n";
    }
    return false;
}

int main() {
    try {
        DbManager mgr("mydb", "database.db", "users.db");

        cout << "=== Mini Database ===\n";
        cout << "First run? Log in with admin / admin123 and change the password with passwd.\n\n";

        if (!loginScreen(mgr)) {
            cout << "Login failed. Exiting.\n";
            return 1;
        }
        cout << "Type help for commands.\n";

        string line;
        while (true) {
            cout << "\n" << (mgr.isLoggedIn() ? mgr.whoAmI() : "guest") << "> ";
            if (!getline(cin, line)) break;
            if (trim(line).empty()) continue;

            try {
                vector<string> tokens = tokenize(line);
                if (tokens.empty()) continue;

                // After logout, the only way forward is to log in again
                if (!mgr.isLoggedIn()) {
                    string c = toLower(tokens[0]);
                    if (c == "exit" || c == "quit") break;
                    if (!loginScreen(mgr)) { cout << "Login failed. Exiting.\n"; break; }
                    continue;
                }

                if (!runCommand(mgr, tokens)) break;
            } catch (const exception& e) {
                cout << "Error: " << e.what() << "\n";
            }
        }

        mgr.save();
        cout << "Goodbye.\n";
    } catch (const exception& e) {
        cout << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}