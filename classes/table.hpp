#pragma once
#include <bits/stdc++.h>
#include "../helper.hpp"
#include "column.hpp"
#include "row.hpp"
using namespace std;

class Table {
private:
    string name;
    vector<Col> cols;
    vector<Row> rows;

    // Schema must have at least one column, no duplicate names, and at most one primary key
    void checkSchema() const {
        if (name.empty()) throw invalid_argument("Table name cannot be empty");
        if (cols.empty()) throw invalid_argument("Table needs at least one column");
        int pkCount = 0;
        for (size_t i = 0; i < cols.size(); i++) {
            if (cols[i].isPrimaryKey()) pkCount++;
            for (size_t j = i + 1; j < cols.size(); j++)
                if (cols[i] == cols[j])
                    throw invalid_argument("Duplicate column: " + cols[i].getName());
        }
        if (pkCount > 1) throw invalid_argument("Only one primary key is allowed");
    }

    // Checks size, types, NOT NULL and primary key uniqueness.
    // skipRow lets update() ignore the row that is being changed.
    void validateRow(const Row& row, int skipRow = -1) const {
        if (row.size() != cols.size())
            throw invalid_argument("Expected " + to_string(cols.size()) + " values, got " + to_string(row.size()));

        for (size_t i = 0; i < cols.size(); i++) {
            if (!cols[i].isValid(row.get(i)))
                throw invalid_argument("Invalid value '" + row.get(i) + "' for column " + cols[i].describe());
        }

        int pk = pkIndex();
        if (pk != -1) {
            for (size_t r = 0; r < rows.size(); r++) {
                if ((int)r == skipRow) continue;
                if (rows[r].matches(pk, row.get(pk)))
                    throw invalid_argument("Duplicate primary key: " + row.get(pk));
            }
        }
    }

    size_t mustFindCol(const string& colName) const {
        int idx = findCol(colName);
        if (idx == -1) throw invalid_argument("No such column: " + colName);
        return idx;
    }

public:
    Table() {}

    Table(const string& name, const vector<Col>& cols) : name(trim(name)), cols(cols) {
        checkSchema();
    }

    // getters 
    string getName() const { return name; }
    const vector<Col>& getCols() const { return cols; }
    const vector<Row>& getRows() const { return rows; }
    size_t colCount() const { return cols.size(); }
    size_t rowCount() const { return rows.size(); }

    // Returns the column index (case-insensitive), or -1 if not found
    int findCol(const string& colName) const {
        string target = toLower(trim(colName));
        for (size_t i = 0; i < cols.size(); i++)
            if (toLower(cols[i].getName()) == target) return i;
        return -1;
    }

    // Returns the primary key column index, or -1 if there is none
    int pkIndex() const {
        for (size_t i = 0; i < cols.size(); i++)
            if (cols[i].isPrimaryKey()) return i;
        return -1;
    }

    // insert
    void insert(const vector<string>& values) {
        Row row(values);
        validateRow(row);
        rows.push_back(row);
    }

    // select 
    vector<Row> selectAll() const { return rows; }

    vector<Row> select(const string& colName, const string& value) const {
        size_t idx = mustFindCol(colName);
        vector<Row> result;
        for (const Row& r : rows)
            if (r.matches(idx, value)) result.push_back(r);
        return result;
    }

    // update
    // Sets setCol = newValue for every row where whereCol == whereValue. Returns rows changed.
    int update(const string& whereCol, const string& whereValue,
               const string& setCol, const string& newValue) {
        size_t w = mustFindCol(whereCol);
        size_t s = mustFindCol(setCol);

        vector<int> hits;
        for (size_t i = 0; i < rows.size(); i++)
            if (rows[i].matches(w, whereValue)) hits.push_back(i);

        if (hits.empty()) return 0;
        if (cols[s].isPrimaryKey() && hits.size() > 1)
            throw invalid_argument("Cannot give the same primary key to multiple rows");

        // Validate every change first so a failure leaves the table untouched
        for (int i : hits) {
            Row copy = rows[i];
            copy.set(s, newValue);
            validateRow(copy, i);
        }
        for (int i : hits) rows[i].set(s, newValue);
        return hits.size();
    }

    // delete 
    // Removes every row where colName == value. Returns rows removed.
    int deleteWhere(const string& colName, const string& value) {
        size_t idx = mustFindCol(colName);
        size_t before = rows.size();
        rows.erase(remove_if(rows.begin(), rows.end(),
                             [&](const Row& r) { return r.matches(idx, value); }),
                   rows.end());
        return before - rows.size();
    }

    // schema changes 
    // Adds a column; every existing row gets defaultValue for it
    void addColumn(const Col& col, const string& defaultValue = "") {
        if (findCol(col.getName()) != -1)
            throw invalid_argument("Column already exists: " + col.getName());
        if (col.isPrimaryKey())
            throw invalid_argument("Cannot add a primary key column to an existing table");
        if (!col.isValid(defaultValue))
            throw invalid_argument("Default value '" + defaultValue + "' is not valid for " + col.describe());

        cols.push_back(col);
        for (Row& r : rows) r.add(defaultValue);
    }

    void dropColumn(const string& colName) {
        size_t idx = mustFindCol(colName);
        if (cols.size() == 1) throw invalid_argument("Cannot drop the only column");
        if (cols[idx].isPrimaryKey()) throw invalid_argument("Cannot drop the primary key column");

        cols.erase(cols.begin() + idx);
        for (Row& r : rows) r.remove(idx);
    }

    // Format:
    // TABLE|name
    // COLS|n
    //   (n column lines)
    // ROWS|m
    //   (m row lines)
    vector<string> serialize() const {
        vector<string> lines;
        lines.push_back("TABLE|" + escape(name));
        lines.push_back("COLS|" + to_string(cols.size()));
        for (const Col& c : cols) lines.push_back(c.serialize());
        lines.push_back("ROWS|" + to_string(rows.size()));
        for (const Row& r : rows) lines.push_back(r.serialize());
        return lines;
    }

    // Reads one table starting at lines[pos] and moves pos past it,
    // so FileManager can call this repeatedly for a file with many tables
    static Table deserialize(const vector<string>& lines, size_t& pos) {
        auto next = [&]() -> const string& {
            if (pos >= lines.size()) throw runtime_error("Unexpected end of file");
            return lines[pos++];
        };

        vector<string> h = splitEscaped(next());
        if (h.size() != 2 || h[0] != "TABLE") throw runtime_error("Corrupt table header");

        Table t;
        t.name = h[1];

        vector<string> c = splitEscaped(next());
        if (c.size() != 2 || c[0] != "COLS") throw runtime_error("Corrupt column header");
        int n = stoi(c[1]);
        for (int i = 0; i < n; i++) t.cols.push_back(Col::deserialize(next()));

        vector<string> r = splitEscaped(next());
        if (r.size() != 2 || r[0] != "ROWS") throw runtime_error("Corrupt row header");
        int m = stoi(r[1]);
        for (int i = 0; i < m; i++) t.rows.push_back(Row::deserialize(next()));

        t.checkSchema();
        return t;
    }

    //  display 
    // Builds an aligned text table from the given rows (all rows by default)
    string format(const vector<Row>& data) const {
        vector<size_t> widths;
        for (const Col& c : cols) widths.push_back(c.getName().size());
        for (const Row& r : data)
            for (size_t i = 0; i < r.size() && i < widths.size(); i++)
                widths[i] = max(widths[i], r.get(i).size());

        size_t total = 1;
        for (size_t w : widths) total += w + 3;
        string line = repeat("-", total) + "\n";

        vector<string> names;
        for (const Col& c : cols) names.push_back(c.getName());

        string out = line + Row(names).format(widths) + "\n" + line;
        for (const Row& r : data) out += r.format(widths) + "\n";
        out += line;
        out += to_string(data.size()) + " row(s)\n";
        return out;
    }

    string toString() const { return format(rows); }

    // Prints the table structure
    string describe() const {
        string out = "Table " + name + "\n";
        for (const Col& c : cols) out += "  " + c.describe() + "\n";
        return out;
    }
};