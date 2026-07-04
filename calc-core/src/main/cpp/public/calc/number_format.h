#ifndef CALC_NUMBER_FORMAT_H
#define CALC_NUMBER_FORMAT_H

#include <string>
#include <calc/big_decimal.h>

namespace calc {

/**
 * Configuration for number display formatting.
 * Controls how calculation results are presented to the user.
 */
struct NumberFormatConfig {
    FormatMode mode = FormatMode::FLOATING_POINT;
    int base = 10;                    ///< Display base: 2, 8, 10, 16
    int significantDigits = 0;        ///< 0 = auto
    int fractionalDigits = 0;         ///< 0 = auto (for FIXED_POINT)
    RoundingMode rounding = RoundingMode::ROUND_TO_NEAREST;
    bool showBasePrefix = true;       ///< Show 0b/0o/0x prefix for non-decimal bases
};

/**
 * Formatter that converts BigDecimal values to display strings
 * according to the current NumberFormatConfig.
 */
class NumberFormatter {
public:
    /// Format a BigDecimal according to the given config
    static std::string format(const BigDecimal& value, const NumberFormatConfig& config);

    /// Format with default config
    static std::string format(const BigDecimal& value);

    /// Get/set global default config
    static NumberFormatConfig& defaultConfig();

    /// Parse a number string that may include base prefix
    /// Supports: 0b (binary), 0o (octal), 0x (hex)
    static BigDecimal parseWithBase(const std::string& input,
                                    int precision = BigDecimal::DEFAULT_PRECISION);

    /// Detect base from prefix
    static int detectBase(const std::string& input);

private:
    static NumberFormatConfig defaultConfig_;
};

} // namespace calc

#endif // CALC_NUMBER_FORMAT_H
