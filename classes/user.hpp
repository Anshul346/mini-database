#pragma once
#include "../helper.hpp"
using namespace std;

class User {
protected:
    string username;
    string passwordHash;

public:
    // Pass hashed = true when loading from a file (the password is already hashed)
    User(const string& username, const string& password, bool hashed = false)
        : username(trim(username)),
          passwordHash(hashed ? password : hashPassword(password)) {
        if (this->username.empty()) throw invalid_argument("Username cannot be empty");
        if (password.empty()) throw invalid_argument("Password cannot be empty");
    }

    virtual ~User() {}

    // ---------- getters ----------
    string getUsername() const { return username; }

    // ---------- password ----------
    bool checkPassword(const string& password) const {
        return passwordHash == hashPassword(password);
    }

    void setPassword(const string& newPassword) {
        if (newPassword.empty()) throw invalid_argument("Password cannot be empty");
        passwordHash = hashPassword(newPassword);
    }

    void changePassword(const string& oldPassword, const string& newPassword) {
        if (!checkPassword(oldPassword)) throw invalid_argument("Wrong password");
        setPassword(newPassword);
    }

    // ---------- role and permissions (implemented by Admin and Regular) ----------
    virtual string getRole() const = 0;
    virtual bool canSelect() const = 0;
    virtual bool canInsert() const = 0;
    virtual bool canUpdate() const = 0;
    virtual bool canDelete() const = 0;
    virtual bool canCreateTable() const = 0;
    virtual bool canDropTable() const = 0;
    virtual bool canAlterTable() const = 0;
    virtual bool canManageUsers() const = 0;

    // Format: ROLE|username|passwordHash
    string serialize() const {
        return escape(getRole()) + "|" + escape(username) + "|" + escape(passwordHash);
    }

    // display 
    string describe() const {
        return username + " (" + getRole() + ")";
    }
};

class Admin : public User {
public:
    Admin(const string& username, const string& password, bool hashed = false)
        : User(username, password, hashed) {}

    string getRole() const override { return "ADMIN"; }

    // Admin can do everything
    bool canSelect() const override { return true; }
    bool canInsert() const override { return true; }
    bool canUpdate() const override { return true; }
    bool canDelete() const override { return true; }
    bool canCreateTable() const override { return true; }
    bool canDropTable() const override { return true; }
    bool canAlterTable() const override { return true; }
    bool canManageUsers() const override { return true; }

    // Admin-only action: reset another user's password
    void resetPassword(User& target, const string& newPassword) const {
        target.setPassword(newPassword);
    }
};

class Regular : public User {
public:
    Regular(const string& username, const string& password, bool hashed = false)
        : User(username, password, hashed) {}

    string getRole() const override { return "REGULAR"; }

    // Regular users can read and change data, but not the structure or the users
    bool canSelect() const override { return true; }
    bool canInsert() const override { return true; }
    bool canUpdate() const override { return true; }
    bool canDelete() const override { return false; }
    bool canCreateTable() const override { return false; }
    bool canDropTable() const override { return false; }
    bool canAlterTable() const override { return false; }
    bool canManageUsers() const override { return false; }
};