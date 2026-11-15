#pragma once
#include "../helper.hpp"
#include "database.hpp"
#include "user.hpp"
using namespace std;

class FileManager {
private:
    string dbFile;
    string userFile;

    // Reads every line of a file (empty lines are kept, they can be valid rows)
    static vector<string> readLines(const string& path) {
        ifstream in(path);
        if (!in) throw runtime_error("Cannot open file: " + path);
        vector<string> lines;
        string line;
        while (getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }

    // Writes to a temp file first, then replaces the real file,
    // so a failed save never leaves a half-written file behind
    static void writeLines(const string& path, const vector<string>& lines) {
        string tmp = path + ".tmp";
        {
            ofstream out(tmp, ios::trunc);
            if (!out) throw runtime_error("Cannot write file: " + tmp);
            for (const string& l : lines) out << l << "\n";
            out.flush();
            if (!out) throw runtime_error("Error while writing file: " + tmp);
        }
        remove(path.c_str());
        if (rename(tmp.c_str(), path.c_str()) != 0)
            throw runtime_error("Cannot replace file: " + path);
    }

    // Builds the right subclass from a saved ROLE|username|hash line
    static unique_ptr<User> makeUser(const string& line) {
        vector<string> p = splitEscaped(line);
        if (p.size() != 3) throw runtime_error("Corrupt user record: " + line);
        if (p[0] == "ADMIN")   return make_unique<Admin>(p[1], p[2], true);
        if (p[0] == "REGULAR") return make_unique<Regular>(p[1], p[2], true);
        throw runtime_error("Unknown user role: " + p[0]);
    }

public:
    FileManager(const string& dbFile = "database.db", const string& userFile = "users.db")
        : dbFile(dbFile), userFile(userFile) {}

    // ---------- getters ----------
    string getDbFile() const { return dbFile; }
    string getUserFile() const { return userFile; }

    // ---------- file checks ----------
    static bool exists(const string& path) {
        ifstream in(path);
        return in.good();
    }

    bool dbFileExists() const { return exists(dbFile); }
    bool userFileExists() const { return exists(userFile); }

    // ---------- database ----------
    void saveDb(const Db& db) const {
        writeLines(dbFile, db.serialize());
    }

    Db loadDb() const {
        vector<string> lines = readLines(dbFile);
        size_t pos = 0;
        return Db::deserialize(lines, pos);
    }

    // ---------- users ----------
    // Format:
    // USERS|n
    //   (n lines, each as written by User::serialize)
    void saveUsers(const vector<unique_ptr<User>>& users) const {
        vector<string> lines;
        lines.push_back("USERS|" + to_string(users.size()));
        for (const auto& u : users) lines.push_back(u->serialize());
        writeLines(userFile, lines);
    }

    vector<unique_ptr<User>> loadUsers() const {
        vector<string> lines = readLines(userFile);
        if (lines.empty()) throw runtime_error("User file is empty");

        vector<string> h = splitEscaped(lines[0]);
        if (h.size() != 2 || h[0] != "USERS") throw runtime_error("Corrupt users header");
        int n = stoi(h[1]);
        if ((int)lines.size() < n + 1) throw runtime_error("User file is missing records");

        vector<unique_ptr<User>> users;
        for (int i = 1; i <= n; i++) users.push_back(makeUser(lines[i]));
        return users;
    }

    // ---------- cleanup ----------
    void deleteFiles() const {
        remove(dbFile.c_str());
        remove(userFile.c_str());
    }
};