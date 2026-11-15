#pragma once
#include "../helper.hpp"
using namespace std;

class Row {
private:
    vector<string> values;

public:
    Row() {}

    Row(const vector<string>& values) : values(values) {}

    // getters 
    int size(){ return values.size(); }
    bool isEmpty(){ return values.empty(); }
    const vector<string>& getValues(){ return values; }

    // Returns the value at a column index
    string get(int index){
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        return values[index];
    }

    // setters 
    void set(int index, const string& value) {
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        values[index] = value;
    }

    // Adds a new value at the end
    void add(const string& value) { values.push_back(value); }

    // Removes the value at an index
    void remove(int index) {
        if (index >= values.size()) throw out_of_range("Row index out of range: " + to_string(index));
        values.erase(values.begin() + index);
    }

    //  searching
    bool matches(int index, const string& value){
        if (index >= values.size()) return false;
        return values[index] == value;
    }

    // Format: value1|value2|value3
    string serialize(){
        vector<string> escaped;
        for (const string& v : values) escaped.push_back(escape(v));
        return join(escaped, "|");
    }

    static Row deserialize(const string& line){
        return Row(splitEscaped(line));
    }

    // display
    string format(const vector<size_t>& widths){
        string out = "| ";
        for (int i = 0; i < values.size(); i++) {
            int w = (i < widths.size()) ? widths[i] : values[i].size();
            out += padRight(values[i], w) + " | ";
        }
        return out;
    }

    string toString(){
        return join(values, ", ");
    }

    bool operator==(const Row& other) const {
        return values == other.values;
    }
};