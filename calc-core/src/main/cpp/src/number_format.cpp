#include <calc/number_format.h>
#include <calc/base_converter.h>

namespace calc {

NumberFormatConfig NumberFormatter::defaultConfig_;

std::string NumberFormatter::format(const BigDecimal& value, const NumberFormatConfig& config) {
    if (value.isNaN()) return "NaN";
    if (value.isInfinite()) return value.isNegative() ? "-Inf" : "Inf";
    if (value.isZero()) return "0";

    std::string prefix;
    if (config.showBasePrefix && config.base != 10) {
        prefix = BaseConverter::basePrefix(config.base);
    }

    std::string numStr;
    if (config.base == 10) {
        numStr = value.toString(config.mode, config.significantDigits, 10);
    } else {
        numStr = value.toBaseString(config.base, config.mode, config.significantDigits);
    }

    return prefix + numStr;
}

std::string NumberFormatter::format(const BigDecimal& value) {
    return format(value, defaultConfig_);
}

NumberFormatConfig& NumberFormatter::defaultConfig() {
    return defaultConfig_;
}

BigDecimal NumberFormatter::parseWithBase(const std::string& input, int precision) {
    int base = detectBase(input);
    std::string numPart = input;

    // Strip prefix
    if (input.size() > 2) {
        std::string prefix = input.substr(0, 2);
        if (prefix == "0b" || prefix == "0B" ||
            prefix == "0o" || prefix == "0O" ||
            prefix == "0x" || prefix == "0X") {
            numPart = input.substr(2);
        }
    }

    return BigDecimal::fromBase(numPart, base, precision);
}

int NumberFormatter::detectBase(const std::string& input) {
    if (input.size() > 2) {
        std::string prefix = input.substr(0, 2);
        if (prefix == "0b" || prefix == "0B") return 2;
        if (prefix == "0o" || prefix == "0O") return 8;
        if (prefix == "0x" || prefix == "0X") return 16;
    }
    return 10;
}

} // namespace calc
