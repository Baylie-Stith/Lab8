// String-based calculator: validates and adds decimal numbers kept as strings.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Valid format: (+|-)A(.B) with at least one digit on both sides of '.'
bool isValidNumber(const string &s) {
    size_t i = 0, n = s.size();
    if (i < n && (s[i] == '+' || s[i] == '-')) i++;
    size_t start = i;
    while (i < n && isDigit(s[i])) i++;
    if (i == start) return false;            // need integer digits
    if (i == n) return true;
    if (s[i] != '.') return false;
    i++;
    size_t fracStart = i;
    while (i < n && isDigit(s[i])) i++;
    return i > fracStart && i == n;          // need fraction digits, nothing after
}

struct Num {
    bool neg;
    string digits;  // integer and fraction digits concatenated
    size_t scale;   // number of fraction digits
};

Num parse(const string &s) {
    Num r{false, "", 0};
    size_t i = 0;
    if (s[i] == '+' || s[i] == '-') { r.neg = (s[i] == '-'); i++; }
    bool inFrac = false;
    for (; i < s.size(); i++) {
        if (s[i] == '.') { inFrac = true; continue; }
        r.digits += s[i];
        if (inFrac) r.scale++;
    }
    return r;
}

// Compare two equal-length digit strings.
int cmpDigits(const string &a, const string &b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;  // equal length: lexicographic == numeric
}

string addDigits(const string &a, const string &b) {  // equal length
    string res(a.size(), '0');
    int carry = 0;
    for (int i = (int)a.size() - 1; i >= 0; i--) {
        int sum = (a[i] - '0') + (b[i] - '0') + carry;
        res[i] = char('0' + sum % 10);
        carry = sum / 10;
    }
    if (carry) res.insert(res.begin(), '1');
    return res;
}

string subDigits(const string &a, const string &b) {  // a >= b, equal length
    string res(a.size(), '0');
    int borrow = 0;
    for (int i = (int)a.size() - 1; i >= 0; i--) {
        int d = (a[i] - '0') - (b[i] - '0') - borrow;
        if (d < 0) { d += 10; borrow = 1; } else borrow = 0;
        res[i] = char('0' + d);
    }
    return res;
}

string addNumbers(const string &x, const string &y) {
    Num a = parse(x), b = parse(y);
    size_t scale = max(a.scale, b.scale);
    a.digits.append(scale - a.scale, '0');
    b.digits.append(scale - b.scale, '0');
    size_t len = max(a.digits.size(), b.digits.size());
    a.digits.insert(0, len - a.digits.size(), '0');
    b.digits.insert(0, len - b.digits.size(), '0');

    string mag;
    bool neg;
    if (a.neg == b.neg) {
        mag = addDigits(a.digits, b.digits);
        neg = a.neg;
    } else {
        int c = cmpDigits(a.digits, b.digits);
        if (c >= 0) { mag = subDigits(a.digits, b.digits); neg = a.neg; }
        else        { mag = subDigits(b.digits, a.digits); neg = b.neg; }
    }

    // Place the decimal point.
    if (mag.size() < scale + 1) mag.insert(0, scale + 1 - mag.size(), '0');
    string intPart = mag.substr(0, mag.size() - scale);
    string fracPart = mag.substr(mag.size() - scale);

    // Normalize: strip leading zeros of integer part, trailing zeros of fraction.
    size_t k = 0;
    while (k + 1 < intPart.size() && intPart[k] == '0') k++;
    intPart = intPart.substr(k);
    while (!fracPart.empty() && fracPart.back() == '0') fracPart.pop_back();

    string result = intPart;
    if (!fracPart.empty()) result += "." + fracPart;
    bool isZero = (intPart == "0" && fracPart.empty());
    if (neg && !isZero) result = "-" + result;
    return result;
}

void processFile(const string &filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "Error: cannot open file \"" << filename << "\"\n";
        return;
    }
    string line;
    int lineNo = 0;
    while (getline(in, line)) {
        lineNo++;
        istringstream iss(line);
        vector<string> tokens;
        string tok;
        while (iss >> tok) tokens.push_back(tok);

        cout << "line " << lineNo << ": ";
        if (tokens.size() != 2) {
            cout << "ERROR: expected exactly two numbers\n";
            continue;
        }
        bool ok1 = isValidNumber(tokens[0]), ok2 = isValidNumber(tokens[1]);
        if (!ok1 || !ok2) {
            cout << "ERROR: invalid number";
            if (!ok1) cout << " \"" << tokens[0] << "\"";
            if (!ok2) cout << " \"" << tokens[1] << "\"";
            cout << "\n";
            continue;
        }
        cout << tokens[0] << " + " << tokens[1] << " = "
             << addNumbers(tokens[0], tokens[1]) << "\n";
    }
}

int main(int argc, char *argv[]) {
    string filename;
    if (argc > 1) filename = argv[1];
    else {
        cout << "Enter input file name: ";
        getline(cin, filename);
    }
    processFile(filename);
    return 0;
}