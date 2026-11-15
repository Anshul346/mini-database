#pragma once
#include "../helper.hpp"
using namespace std;

class Row {
private:
    vector<string> values;

public:
    Row() {}

    Row(const vector<string>& values) : values(values) {}

    // ---------- getters ----------
    size_t size() const { return values.size(); }
    bool isEmpty() const { return values.empty(); }
    const vector<string>& getValues() const { return values; }

    // Returns the value at a column index (bounds checked)
    string get(size_t index) const {
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        return values[index];
    }

    // ---------- setters ----------
    void set(size_t index, const string& value) {
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        values[index] = value;
    }

    // Adds a new value at the end (used when a column is added to a table)
    void add(const string& value) { values.push_back(value); }

    // Removes the value at an index (used when a column is dropped from a table)
    void remove(size_t index) {
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        values.erase(values.begin() + index);
    }

    // ---------- searching ----------
    // Checks whether the value at a column index equals the given value
    bool matches(size_t index, const string& value) const {
        if (index >= values.size()) return false;
        return values[index] == value;
    }

    // ---------- file persistence (used by FileManager) ----------
    // Format: value1|value2|value3
    string serialize() const {
        vector<string> escaped;
        for (const string& v : values) escaped.push_back(escape(v));
        return join(escaped, "|");
    }

    static Row deserialize(const string& line) {
        return Row(splitEscaped(line));
    }

    // ---------- display ----------
    // Prints the row padded to the given column widths
    string format(const vector<size_t>& widths) const {
        string out = "| ";
        for (size_t i = 0; i < values.size(); i++) {
            size_t w = (i < widths.size()) ? widths[i] : values[i].size();
            out += padRight(values[i], w) + " | ";
        }
        return out;
    }

    string toString() const {
        return join(values, ", ");
    }

    bool operator==(const Row& other) const {
        return values == other.values;
    }
};