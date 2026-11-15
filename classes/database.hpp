#pragma once
#include "../helper.hpp"
#include "column.hpp"
#include "row.hpp"
#include "table.hpp"
using namespace std;

class Db {
private:
    string name;
    vector<Table> tables;

    size_t mustFindTable(const string& tableName) const {
        int idx = findTable(tableName);
        if (idx == -1) throw invalid_argument("No such table: " + tableName);
        return idx;
    }

public:
    Db() {}

    Db(const string& name) : name(trim(name)) {
        if (this->name.empty()) throw invalid_argument("Database name cannot be empty");
    }

    // ---------- getters ----------
    string getName() const { return name; }
    const vector<Table>& getTables() const { return tables; }
    size_t tableCount() const { return tables.size(); }

    // Returns the table index (case-insensitive), or -1 if not found
    int findTable(const string& tableName) const {
        string target = toLower(trim(tableName));
        for (size_t i = 0; i < tables.size(); i++)
            if (toLower(tables[i].getName()) == target) return i;
        return -1;
    }

    bool hasTable(const string& tableName) const { return findTable(tableName) != -1; }

    // Returns a reference to the table so callers can insert, select, update, etc.
    Table& getTable(const string& tableName) {
        return tables[mustFindTable(tableName)];
    }

    const Table& getTable(const string& tableName) const {
        return tables[mustFindTable(tableName)];
    }

    vector<string> listTables() const {
        vector<string> names;
        for (const Table& t : tables) names.push_back(t.getName());
        return names;
    }

    // ---------- table management ----------
    void createTable(const string& tableName, const vector<Col>& cols) {
        if (hasTable(tableName))
            throw invalid_argument("Table already exists: " + tableName);
        tables.push_back(Table(tableName, cols));
    }

    void dropTable(const string& tableName) {
        size_t idx = mustFindTable(tableName);
        tables.erase(tables.begin() + idx);
    }

    void renameTable(const string& oldName, const string& newName) {
        size_t idx = mustFindTable(oldName);
        if (hasTable(newName) && findTable(newName) != (int)idx)
            throw invalid_argument("Table already exists: " + newName);
        // Table has no setName, so rebuild it with the new name and copy the rows over
        Table old = tables[idx];
        Table renamed(newName, old.getCols());
        for (const Row& r : old.getRows()) renamed.insert(r.getValues());
        tables[idx] = renamed;
    }

    // ---------- file persistence (used by FileManager) ----------
    // Format:
    // DB|name
    // TABLES|n
    //   (n tables, each as written by Table::serialize)
    vector<string> serialize() const {
        vector<string> lines;
        lines.push_back("DB|" + escape(name));
        lines.push_back("TABLES|" + to_string(tables.size()));
        for (const Table& t : tables) {
            vector<string> tl = t.serialize();
            lines.insert(lines.end(), tl.begin(), tl.end());
        }
        return lines;
    }

    static Db deserialize(const vector<string>& lines, size_t& pos) {
        auto next = [&]() -> const string& {
            if (pos >= lines.size()) throw runtime_error("Unexpected end of file");
            return lines[pos++];
        };

        vector<string> h = splitEscaped(next());
        if (h.size() != 2 || h[0] != "DB") throw runtime_error("Corrupt database header");

        Db db(h[1]);

        vector<string> t = splitEscaped(next());
        if (t.size() != 2 || t[0] != "TABLES") throw runtime_error("Corrupt tables header");
        int n = stoi(t[1]);
        for (int i = 0; i < n; i++) db.tables.push_back(Table::deserialize(lines, pos));

        return db;
    }

    // ---------- display ----------
    string toString() const {
        string out = "Database " + name + " (" + to_string(tables.size()) + " table(s))\n";
        for (const Table& t : tables)
            out += "  " + t.getName() + " - " + to_string(t.colCount()) + " col(s), " +
                   to_string(t.rowCount()) + " row(s)\n";
        return out;
    }
};