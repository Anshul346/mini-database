#pragma once
#include "../helper.hpp"
using namespace std;

enum ColType { INT, DOUBLE, STRING };

class Col {
private:
    string name;
    ColType type;
    bool primaryKey;
    bool notNull;

public:
    Col() : name(""), type(STRING), primaryKey(false), notNull(false) {}

    Col(const string& name, ColType type, bool primaryKey = false, bool notNull = false)
        : name(trim(name)), type(type), primaryKey(primaryKey), notNull(notNull || primaryKey) {}

    // getters 
    string getName() const { return name; }
    ColType getType() const { return type; }
    bool isPrimaryKey() const { return primaryKey; }
    bool isNotNull() const { return notNull; }

    string getTypeName() const { return typeToString(type); }

    // setters
    void setName(const string& newName) { name = trim(newName); }

    // Checks whether a value is allowed in this column
    bool isValid(const string& value) const {
        if (value.empty()) return !notNull;
        switch (type) {
            case INT:    return isInteger(value);
            case DOUBLE: return isNumber(value);
            case STRING: return true;
        }
        return false;
    }

    static string typeToString(ColType t) {
        switch (t) {
            case INT:    return "INT";
            case DOUBLE: return "DOUBLE";
            case STRING: return "STRING";
        }
        return "STRING";
    }

    static ColType typeFromString(const string& s) {
        string u = toUpper(trim(s));
        if (u == "INT") return INT;
        if (u == "DOUBLE") return DOUBLE;
        if (u == "STRING") return STRING;
        throw invalid_argument("Unknown column type: " + s);
    }

    // Format: name|TYPE|pk|nn
    string serialize() const {
        return escape(name) + "|" + getTypeName() + "|" +
               (primaryKey ? "1" : "0") + "|" + (notNull ? "1" : "0");
    }

    static Col deserialize(const string& line) {
        vector<string> p = splitEscaped(line);
        if (p.size() != 4) throw runtime_error("Corrupt column record: " + line);
        return Col(p[0], typeFromString(p[1]), p[2] == "1", p[3] == "1");
    }

    // display 
    string describe() const {
        string s = name + " (" + getTypeName() + ")";
        if (primaryKey) s += " [PK]";
        else if (notNull) s += " [NOT NULL]";
        return s;
    }

    bool operator==(const Col& other) const {
        return toLower(name) == toLower(other.name);
    }
};