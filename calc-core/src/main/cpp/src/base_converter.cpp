#include <calc/base_converter.h>
#include <stdexcept>
#include <algorithm>

namespace calc {

// Detect base from prefix: 0b→2, 0o→8, 0x→16, default→10
static int detectBase(const std::string& input) {
    if (input.size() > 2) {
        std::string prefix = input.substr(0, 2);
        if (prefix == "0b" || prefix == "0B") return 2;
        if (prefix == "0o" || prefix == "0O") return 8;
        if (prefix == "0x" || prefix == "0X") return 16;
    }
    return 10;
}

std::string BaseConverter::toString(const BigDecimal& value, int base,
                                      FormatMode mode, int digits) {
    if (base < 2 || base > 16) {
        throw std::invalid_argument("Base must be between 2 and 16");
    }
    return value.toBaseString(base, mode, digits);
}

BigDecimal BaseConverter::fromString(const std::string& input, int base, int precision) {
    int b = base;
    std::string numPart = input;

    if (base == 0) {
        b = detectBase(input);
    }

    // Strip prefix
    if (input.size() > 2) {
        std::string prefix = input.substr(0, 2);
        if (prefix == "0b" || prefix == "0B" ||
            prefix == "0o" || prefix == "0O" ||
            prefix == "0x" || prefix == "0X") {
            numPart = input.substr(2);
        }
    }

    return BigDecimal::fromBase(numPart, b, precision);
}

std::string BaseConverter::integerToBase(int64_t value, int base) {
    if (value == 0) return "0";

    bool negative = value < 0;
    uint64_t absVal = negative ? -static_cast<uint64_t>(value) : static_cast<uint64_t>(value);

    std::string result;
    while (absVal > 0) {
        result += digitChar(absVal % base);
        absVal /= base;
    }

    if (negative) result += '-';
    std::reverse(result.begin(), result.end());
    return result;
}

std::string BaseConverter::fractionToBase(const BigDecimal& frac, int base, int maxDigits) {
    std::string result;
    BigDecimal current = frac;
    BigDecimal baseBD(base);
    BigDecimal zero(0);

    for (int i = 0; i < maxDigits; i++) {
        if (current.isZero()) break;
        BigDecimal product = current.mul(baseBD);
        int64_t intPart = product.toInt64();
        result += digitChar(static_cast<int>(intPart));
        current = product.sub(BigDecimal(intPart));
    }

    return result;
}

std::string BaseConverter::basePrefix(int base) {
    switch (base) {
        case 2:  return "0b";
        case 8:  return "0o";
        case 16: return "0x";
        default: return "";
    }
}

bool BaseConverter::isValidInBase(const std::string& input, int base) {
    for (char c : input) {
        int val = digitValue(c);
        if (val < 0 || val >= base) return false;
    }
    return true;
}

char BaseConverter::digitChar(int digit) {
    if (digit < 10) return '0' + digit;
    return 'a' + (digit - 10);
}

int BaseConverter::digitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace calc
