#pragma once
#include "helper.hpp"
#include "classes/column.hpp"
#include "classes/row.hpp"
#include "classes/table.hpp"
#include "classes/database.hpp"
#include "classes/user.hpp"
#include "classes/fileManager.hpp"
using namespace std;

class DbManager {
private:
    FileManager fileManager;
    Db db;
    vector<unique_ptr<User>> users;
    User* currentUser;   // points to an object inside users, not owned

    // ---------- permission helpers ----------
    void requireLogin() const {
        if (!currentUser) throw runtime_error("Please log in first");
    }

    void require(bool allowed, const string& action) const {
        requireLogin();
        if (!allowed) throw runtime_error("Permission denied: " + currentUser->describe() + " cannot " + action);
    }

    int findUser(const string& username) const {
        string target = trim(username);
        for (size_t i = 0; i < users.size(); i++)
            if (users[i]->getUsername() == target) return i;
        return -1;
    }

    int adminCount() const {
        int c = 0;
        for (const auto& u : users)
            if (u->getRole() == "ADMIN") c++;
        return c;
    }

    // ---------- startup ----------
    // Loads saved data if the files exist, otherwise creates a fresh database
    // with a default admin (username: admin, password: admin123)
    void load() {
        if (fileManager.dbFileExists()) db = fileManager.loadDb();
        if (fileManager.userFileExists()) users = fileManager.loadUsers();

        if (users.empty()) {
            users.push_back(make_unique<Admin>("admin", "admin123"));
            save();
        }
    }

public:
    DbManager(const string& dbName = "mydb",
              const string& dbFile = "database.db",
              const string& userFile = "users.db")
        : fileManager(dbFile, userFile), db(dbName), currentUser(nullptr) {
        load();
    }

    // ---------- saving ----------
    void save() const {
        fileManager.saveDb(db);
        fileManager.saveUsers(users);
    }

    // ---------- login / logout ----------
    bool login(const string& username, const string& password) {
        int idx = findUser(username);
        if (idx == -1 || !users[idx]->checkPassword(password)) return false;
        currentUser = users[idx].get();
        return true;
    }

    void logout() { currentUser = nullptr; }

    bool isLoggedIn() const { return currentUser != nullptr; }

    string whoAmI() const {
        return currentUser ? currentUser->describe() : "not logged in";
    }

    // ---------- user management ----------
    void addUser(const string& username, const string& password, const string& role) {
        require(currentUser && currentUser->canManageUsers(), "manage users");
        if (findUser(username) != -1) throw invalid_argument("User already exists: " + username);

        string r = toUpper(trim(role));
        if (r == "ADMIN")        users.push_back(make_unique<Admin>(username, password));
        else if (r == "REGULAR") users.push_back(make_unique<Regular>(username, password));
        else throw invalid_argument("Role must be ADMIN or REGULAR");
        save();
    }

    void removeUser(const string& username) {
        require(currentUser && currentUser->canManageUsers(), "manage users");
        int idx = findUser(username);
        if (idx == -1) throw invalid_argument("No such user: " + username);
        if (users[idx].get() == currentUser) throw invalid_argument("You cannot remove yourself");
        if (users[idx]->getRole() == "ADMIN" && adminCount() == 1)
            throw invalid_argument("Cannot remove the last admin");
        users.erase(users.begin() + idx);
        save();
    }

    vector<string> listUsers() const {
        require(currentUser && currentUser->canManageUsers(), "manage users");
        vector<string> out;
        for (const auto& u : users) out.push_back(u->describe());
        return out;
    }

    void changeOwnPassword(const string& oldPassword, const string& newPassword) {
        requireLogin();
        currentUser->changePassword(oldPassword, newPassword);
        save();
    }

    // Admin-only: uses Admin::resetPassword
    void resetPassword(const string& username, const string& newPassword) {
        require(currentUser && currentUser->canManageUsers(), "manage users");
        int idx = findUser(username);
        if (idx == -1) throw invalid_argument("No such user: " + username);
        Admin* admin = dynamic_cast<Admin*>(currentUser);
        if (!admin) throw runtime_error("Only an admin can reset passwords");
        admin->resetPassword(*users[idx], newPassword);
        save();
    }

    // ---------- table structure ----------
    void createTable(const string& tableName, const vector<Col>& cols) {
        require(currentUser && currentUser->canCreateTable(), "create tables");
        db.createTable(tableName, cols);
        save();
    }

    void dropTable(const string& tableName) {
        require(currentUser && currentUser->canDropTable(), "drop tables");
        db.dropTable(tableName);
        save();
    }

    void addColumn(const string& tableName, const Col& col, const string& defaultValue = "") {
        require(currentUser && currentUser->canAlterTable(), "alter tables");
        db.getTable(tableName).addColumn(col, defaultValue);
        save();
    }

    void dropColumn(const string& tableName, const string& colName) {
        require(currentUser && currentUser->canAlterTable(), "alter tables");
        db.getTable(tableName).dropColumn(colName);
        save();
    }

    // ---------- data operations ----------
    void insert(const string& tableName, const vector<string>& values) {
        require(currentUser && currentUser->canInsert(), "insert data");
        db.getTable(tableName).insert(values);
        save();
    }

    string select(const string& tableName) const {
        require(currentUser && currentUser->canSelect(), "select data");
        const Table& t = db.getTable(tableName);
        return t.format(t.selectAll());
    }

    string select(const string& tableName, const string& colName, const string& value) const {
        require(currentUser && currentUser->canSelect(), "select data");
        const Table& t = db.getTable(tableName);
        return t.format(t.select(colName, value));
    }

    int update(const string& tableName, const string& whereCol, const string& whereValue,
               const string& setCol, const string& newValue) {
        require(currentUser && currentUser->canUpdate(), "update data");
        int n = db.getTable(tableName).update(whereCol, whereValue, setCol, newValue);
        save();
        return n;
    }

    int deleteWhere(const string& tableName, const string& colName, const string& value) {
        require(currentUser && currentUser->canDelete(), "delete data");
        int n = db.getTable(tableName).deleteWhere(colName, value);
        save();
        return n;
    }

    // ---------- info ----------
    string showTables() const {
        require(currentUser && currentUser->canSelect(), "view tables");
        return db.toString();
    }

    string describeTable(const string& tableName) const {
        require(currentUser && currentUser->canSelect(), "view tables");
        return db.getTable(tableName).describe();
    }
};