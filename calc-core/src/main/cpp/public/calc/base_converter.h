#ifndef CALC_BASE_CONVERTER_H
#define CALC_BASE_CONVERTER_H

#include <string>
#include <calc/big_decimal.h>

namespace calc {

/**
 * Base conversion utilities.
 * Converts BigDecimal values to/from string representations in bases 2, 8, 10, 16.
 * Handles both integer and fractional parts.
 */
class BaseConverter {
public:
    /// Convert BigDecimal to string in target base
    /// @param value The number to convert
    /// @param base Target base (2, 8, 10, 16)
    /// @param mode Format mode for the output
    /// @param digits Number of digits (0 = auto)
    /// @return String representation in the target base
    static std::string toString(const BigDecimal& value, int base,
                                FormatMode mode = FormatMode::FLOATING_POINT,
                                int digits = 0);

    /// Parse a string in a given base to BigDecimal
    /// @param input The string to parse
    /// @param base Source base (2, 8, 10, 16), or 0 for auto-detect
    /// @param precision Precision for the result
    /// @return Parsed BigDecimal
    static BigDecimal fromString(const std::string& input, int base = 0,
                                  int precision = BigDecimal::DEFAULT_PRECISION);

    /// Convert integer part to target base string
    static std::string integerToBase(int64_t value, int base);

    /// Convert fractional part to target base string
    /// @param frac Fractional part (0 <= frac < 1)
    /// @param base Target base
    /// @param maxDigits Maximum digits to output
    static std::string fractionToBase(const BigDecimal& frac, int base, int maxDigits = 50);

    /// Get the prefix string for a base ("0b", "0o", "0x", or "")
    static std::string basePrefix(int base);

    /// Validate a string as a valid number in the given base
    static bool isValidInBase(const std::string& input, int base);

    /// Digit characters for each base
    static char digitChar(int digit);
    static int digitValue(char c);
};

} // namespace calc

#endif // CALC_BASE_CONVERTER_H
