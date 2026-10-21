// ============================================================================
// String-based calculator
//
// Reads pairs of numbers from a file (one pair per line), checks that each
// number is a valid decimal number, and adds them. Every number stays a
// string the whole time: no stod/stoi/atof, no try/catch. Only single digits,
// carries, borrows and indices are ever stored as ints.
// ============================================================================

#include <fstream>   // ifstream: reading the input file
#include <iostream>  // cout, cin, cerr: console input/output
#include <sstream>   // istringstream: splitting a line into tokens
#include <string>    // string
#include <vector>    // vector: holds the tokens found on a line

using namespace std;

// ----------------------------------------------------------------------------
// isDigit: true if c is one of the characters '0' through '9'.
// Written by hand instead of using a library function.
// ----------------------------------------------------------------------------
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

// ----------------------------------------------------------------------------
// isValidNumber: checks the string against the format (+|-)A(.B).
// Walks through the string once with index i, never converting to a number.
// ----------------------------------------------------------------------------
bool isValidNumber(const string &s) {
    size_t i = 0, n = s.size();

    // Step 1: skip one optional sign character.
    if (i < n && (s[i] == '+' || s[i] == '-')) i++;

    // Step 2: consume the integer part (the digits before any decimal point).
    size_t start = i;
    while (i < n && isDigit(s[i])) i++;
    if (i == start) return false;   // no digits at all (e.g. "A", "+", ".5", "+-1")

    // Step 3: if the string ends here it is a plain integer, which is valid.
    if (i == n) return true;

    // Step 4: anything left must be a '.' followed by digits.
    // Anything else (e.g. 'e' in "1e3") makes the number invalid.
    if (s[i] != '.') return false;
    i++;  // move past the '.'

    // Step 5: consume the fractional digits.
    size_t fracStart = i;
    while (i < n && isDigit(s[i])) i++;

    // Valid only if there was at least one digit after '.' (rejects "-5.")
    // and we reached the end of the string (rejects "2.3.4" and "-5.-5").
    return i > fracStart && i == n;
}

// ----------------------------------------------------------------------------
// Num: a parsed number, still stored as text.
//   neg    - true if the number is negative
//   digits - all digits with the decimal point removed ("123.45" -> "12345")
//   scale  - how many of those digits come after the decimal point (2 here)
// Together these describe the value: (+/-) digits / 10^scale
// ----------------------------------------------------------------------------
struct Num {
    bool neg;
    string digits;
    size_t scale;
};

// ----------------------------------------------------------------------------
// parse: splits an already-validated string into sign, digits and scale.
// ----------------------------------------------------------------------------
Num parse(const string &s) {
    Num r{false, "", 0};
    size_t i = 0;

    // Read the optional sign.
    if (s[i] == '+' || s[i] == '-') {
        r.neg = (s[i] == '-');
        i++;
    }

    // Copy every digit; skip the '.', but remember we are past it so we can
    // count how many digits come after it (that count is the scale).
    bool inFrac = false;
    for (; i < s.size(); i++) {
        if (s[i] == '.') {
            inFrac = true;
            continue;
        }
        r.digits += s[i];
        if (inFrac) r.scale++;
    }
    return r;
}

// ----------------------------------------------------------------------------
// cmpDigits: compares two digit strings of EQUAL length.
// Returns -1 if a < b, 0 if equal, 1 if a > b.
// With equal lengths (leading zeros included), comparing character by
// character gives the same answer as comparing the numeric values.
// ----------------------------------------------------------------------------
int cmpDigits(const string &a, const string &b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

// ----------------------------------------------------------------------------
// addDigits: adds two non-negative digit strings of equal length, the way you
// add on paper: right to left, carrying into the next column.
// ----------------------------------------------------------------------------
string addDigits(const string &a, const string &b) {
    string res(a.size(), '0');  // result starts as all zeros, same length
    int carry = 0;              // carry from the previous (right) column

    for (int i = (int)a.size() - 1; i >= 0; i--) {
        // Add the two digits in this column plus any carry.
        // (c - '0') turns a digit character into its value 0..9.
        int sum = (a[i] - '0') + (b[i] - '0') + carry;
        res[i] = char('0' + sum % 10);  // the digit to write in this column
        carry = sum / 10;               // 0 or 1, carried to the next column
    }

    // A leftover carry means the answer is one digit longer (999 + 1 = 1000).
    if (carry) res.insert(res.begin(), '1');
    return res;
}

// ----------------------------------------------------------------------------
// subDigits: computes a - b for non-negative digit strings of equal length.
// Requires a >= b so the result is never negative. Right to left with borrow.
// ----------------------------------------------------------------------------
string subDigits(const string &a, const string &b) {
    string res(a.size(), '0');
    int borrow = 0;  // 1 if the previous (right) column had to borrow from us

    for (int i = (int)a.size() - 1; i >= 0; i--) {
        int d = (a[i] - '0') - (b[i] - '0') - borrow;
        if (d < 0) {      // not enough in this column: borrow 10 from the left
            d += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        res[i] = char('0' + d);
    }
    return res;
}

// ----------------------------------------------------------------------------
// addNumbers: adds two valid number strings and returns the sum as a string.
// ----------------------------------------------------------------------------
string addNumbers(const string &x, const string &y) {
    Num a = parse(x), b = parse(y);

    // Step 2a: give both the same number of fractional digits by appending
    // zeros (1.2 + 3.45 -> 1.20 + 3.45, i.e. "120" and "345", scale 2).
    size_t scale = max(a.scale, b.scale);
    a.digits.append(scale - a.scale, '0');
    b.digits.append(scale - b.scale, '0');

    // Step 2b: give both the same total length by prepending zeros, so the
    // digit-by-digit helpers can line up columns.
    size_t len = max(a.digits.size(), b.digits.size());
    a.digits.insert(0, len - a.digits.size(), '0');
    b.digits.insert(0, len - b.digits.size(), '0');

    string mag;  // magnitude (absolute value) of the result, as digits
    bool neg;    // sign of the result

    if (a.neg == b.neg) {
        // Same sign: (+a) + (+b) or (-a) + (-b). Add magnitudes, keep sign.
        mag = addDigits(a.digits, b.digits);
        neg = a.neg;
    } else {
        // Different signs: the result's size is the difference of the
        // magnitudes, and its sign comes from whichever magnitude is bigger.
        int c = cmpDigits(a.digits, b.digits);
        if (c >= 0) {
            mag = subDigits(a.digits, b.digits);
            neg = a.neg;
        } else {
            mag = subDigits(b.digits, a.digits);
            neg = b.neg;
        }
    }

    // Step 4a: restore the decimal point. We need at least one digit before
    // it, so pad with leading zeros if needed (e.g. "5" with scale 3 -> "0005"
    // -> "0.005").
    if (mag.size() < scale + 1) mag.insert(0, scale + 1 - mag.size(), '0');
    string intPart  = mag.substr(0, mag.size() - scale);  // digits before '.'
    string fracPart = mag.substr(mag.size() - scale);     // digits after '.'

    // Step 4b: tidy up. Remove leading zeros from the integer part (but keep
    // at least one digit) and trailing zeros from the fraction.
    size_t k = 0;
    while (k + 1 < intPart.size() && intPart[k] == '0') k++;
    intPart = intPart.substr(k);
    while (!fracPart.empty() && fracPart.back() == '0') fracPart.pop_back();

    // Build the final text; only include '.' if a fraction remains.
    string result = intPart;
    if (!fracPart.empty()) result += "." + fracPart;

    // Add '-' only if negative and the value is not zero (so we never print
    // "-0", e.g. for +123 + -123).
    bool isZero = (intPart == "0" && fracPart.empty());
    if (neg && !isZero) result = "-" + result;
    return result;
}

// ----------------------------------------------------------------------------
// processFile: reads the file line by line. For each line it checks that
// there are exactly two tokens, validates both, and prints either an error
// or the sum. Bad lines never stop the program; it moves on to the next one.
// ----------------------------------------------------------------------------
void processFile(const string &filename) {
    ifstream in(filename);
    if (!in) {  // file missing or unreadable
        cerr << "Error: cannot open file \"" << filename << "\"\n";
        return;
    }

    string line;
    int lineNo = 0;  // 1-based line number used in the output

    while (getline(in, line)) {
        lineNo++;

        // Split the line on whitespace into tokens.
        istringstream iss(line);
        vector<string> tokens;
        string tok;
        while (iss >> tok) tokens.push_back(tok);

        cout << "line " << lineNo << ": ";

        // Requirement: exactly two values per line.
        if (tokens.size() != 2) {
            cout << "ERROR: expected exactly two numbers\n";
            continue;  // go to next line
        }

        // Validate both numbers; report whichever ones are invalid.
        bool ok1 = isValidNumber(tokens[0]);
        bool ok2 = isValidNumber(tokens[1]);
        if (!ok1 || !ok2) {
            cout << "ERROR: invalid number";
            if (!ok1) cout << " \"" << tokens[0] << "\"";
            if (!ok2) cout << " \"" << tokens[1] << "\"";
            cout << "\n";
            continue;
        }

        // Both valid: print the original inputs and their sum.
        cout << tokens[0] << " + " << tokens[1] << " = "
             << addNumbers(tokens[0], tokens[1]) << "\n";
    }
}

// ----------------------------------------------------------------------------
// main: gets the input filename (from the command line if given, otherwise
// by asking the user) and processes it.
// ----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    string filename;
    if (argc > 1) {
        filename = argv[1];
    } else {
        cout << "Enter input file name: ";
        getline(cin, filename);
    }
    processFile(filename);
    return 0;
}