#pragma once

#include <algorithm>
#include <cmath>

// internal header containing various text related helper methods

inline bool isASCII(char c) {
    return static_cast<unsigned char>(c) < 128;
}

inline bool isPrintable(char c) {
    return c >= 33 && c <= 126;
}

inline bool isWhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
        case '\n':
            return true;
    }
    return false;
}

inline bool isHorizontalWhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\f':
            return true;
    }
    return false;
}

inline bool isNewline(char c) {
    return c == '\r' || c == '\n';
}

inline bool isDecimalDigit(char c) {
    return c >= '0' && c <= '9';
}

inline bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

inline bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

inline bool isAlphaNumeric(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isLogicDigit(char c) {
    switch (c) {
        case 'z':
        case 'Z':
        case '?':
        case 'x':
        case 'X':
            return true;
        default:
            return false;
    }
}

inline uint32_t getDigitValue(char c) {
    return c - '0';
}

inline uint32_t getHexDigitValue(char c) {
    if (c <= '9')
        return c - '0';
    if (c <= 'F')
        return 10 + c - 'A';
    return 10 + c - 'a';
}

// returns the number of bytes to skip after reading a UTF-8 char
inline int utf8SeqBytes(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    if ((uc & (3 << 6)) == 0)
        return 0;
    if ((uc & (1 << 5)) == 0)
        return 1;
    if ((uc & (1 << 4)) == 0)
        return 2;
    if ((uc & (1 << 3)) == 0)
        return 3;

    // 5 and 6 byte sequences are disallowed by the UTF-8 spec
    return 0;
}

static const int MaxMantissaDigits = 18;

inline bool composeDouble(double fraction, int exp, double& result) {
    static const double powersOf10[] = {
        10.0,
        100.0,
        1.0e4,
        1.0e8,
        1.0e16,
        1.0e32,
        1.0e64,
        1.0e128,
        1.0e256
    };

    bool neg = false;
    if (exp < 0) {
        neg = true;
        exp = -exp;
    }

    static const int MaxExponent = 511;
    if (exp > MaxExponent)
        exp = MaxExponent;

    double dblExp = 1.0;
    for (auto d = powersOf10; exp != 0; exp >>= 1, d++) {
        if (exp & 0x1)
            dblExp *= *d;
    }

    if (neg)
        fraction /= dblExp;
    else
        fraction *= dblExp;

    result = fraction;
    return std::isfinite(result);
}

inline double computeRealValue(uint64_t value, int decPoint, int digits, uint64_t expValue, bool negative) {
    int fracExp = decPoint - std::min(digits, MaxMantissaDigits);
    int exp;
    if (negative)
        exp = fracExp - int(expValue);
    else
        exp = fracExp + int(expValue);

    double result;
    composeDouble(double(value), exp, result);

    return result;
}
