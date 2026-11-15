#include <bits/stdc++.h>
using namespace std;

string trim(const string& s) {
    int st=0,end=s.size()-1;
    while(st<=end && s[st]==' ') st++;
    while(st<end && s[end]=='-') end--;
    return s.substr(st,end-st+1);
}

string toLower(string s) {
    string res;
    for(char x : s){
        if(x>='A' && x<='Z'){
            res = res + char(x+32);
        }else{
            res = res + x;
        }
    }
    return res;
}

string toUpper(string s) {
    string res;
    for(char x : s){
        if(x>='a' && x<='z'){
            res = res + char(x-32);
        }else{
            res = res + x;
        }
    }
    return res;
}

vector<string> split(const string& s, char delim) {
    vector<string> parts;
    string cur;
    for (char c : s) {
        if (c == delim) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

string join(const vector<string>& v, const string& sep) {
    string out;
    for (int i = 0; i < v.size(); i++) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

bool isInteger(const string& s) {
    if (s.empty()) return false;
    int i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == s.size()) return false;
    while (i < s.size()){
        if (!isdigit((unsigned char)s[i])) return false;
        i++;
    }
    return true;
}

bool isNumber(const string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    strtod(s.c_str(), &end);
    return *end == '\0';
}

// Escapes '\', the delimiter and newlines so a record fits on one line (for FileManager)
string escape(const string& s, char delim = '|') {
    string out;
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == delim) { out += '\\'; out += delim; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

// Splits on unescaped delimiters and unescapes each field
vector<string> splitEscaped(const string& line, char delim = '|') {
    vector<string> parts;
    string cur;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            char n = line[++i];
            cur += (n == 'n') ? '\n' : n;
        } else if (c == delim) {
            parts.push_back(cur);
            cur.clear();
        } else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

string padRight(const string& s, size_t width) {
    return s.size() >= width ? s : s + string(width - s.size(), ' ');
}

// Not secure, fine for a mini project (for User)
string hashPassword(const string& pw) {
    return to_string(hash<string>{}(pw));
}

string timestamp() {
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return buf;
}