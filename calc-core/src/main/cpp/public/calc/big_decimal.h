#ifndef CALC_BIG_DECIMAL_H
#define CALC_BIG_DECIMAL_H

#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include <mpfr.h>

namespace calc {

/**
 * Number formatting mode for display output.
 */
enum class FormatMode {
    SCIENTIFIC,          ///< 1.23456789e+5
    FIXED_POINT,         ///< 123456.789000
    SIGNIFICANT_DIGITS,  ///< 1.23456789 (specified sig digits)
    FLOATING_POINT       ///< Auto-select best representation
};

/**
 * Rounding mode for arithmetic operations.
 */
enum class RoundingMode {
    ROUND_TO_NEAREST,
    ROUND_TOWARD_ZERO,
    ROUND_TOWARD_POS_INF,
    ROUND_TOWARD_NEG_INF,
    ROUND_AWAY_FROM_ZERO
};

/**
 * High-precision decimal number based on MPFR.
 * Default precision: 256 bits (~77 significant decimal digits).
 */
class BigDecimal {
public:
    static constexpr int DEFAULT_PRECISION = 256;

    BigDecimal();
    explicit BigDecimal(int precision);
    BigDecimal(const std::string& value, int precision = DEFAULT_PRECISION);
    BigDecimal(double value, int precision = DEFAULT_PRECISION);
    BigDecimal(int64_t value, int precision = DEFAULT_PRECISION);
    BigDecimal(const BigDecimal& other);
    BigDecimal(BigDecimal&& other) noexcept;
    ~BigDecimal();

    BigDecimal& operator=(const BigDecimal& other);
    BigDecimal& operator=(BigDecimal&& other) noexcept;
    BigDecimal& operator=(const std::string& value);

    // Arithmetic
    BigDecimal add(const BigDecimal& rhs, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal sub(const BigDecimal& rhs, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal mul(const BigDecimal& rhs, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal div(const BigDecimal& rhs, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal mod(const BigDecimal& rhs, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal neg() const;
    BigDecimal abs() const;
    BigDecimal floor() const;
    BigDecimal ceil() const;

    /// Power with fast exponentiation for integer exponents
    BigDecimal pow(int64_t exponent, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;
    BigDecimal pow(const BigDecimal& exponent, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;

    /// N-th root
    BigDecimal nthRoot(unsigned long n, RoundingMode rm = RoundingMode::ROUND_TO_NEAREST) const;

    // Comparison
    int compareTo(const BigDecimal& rhs) const;
    bool operator<(const BigDecimal& rhs) const;
    bool operator<=(const BigDecimal& rhs) const;
    bool operator>(const BigDecimal& rhs) const;
    bool operator>=(const BigDecimal& rhs) const;
    bool operator==(const BigDecimal& rhs) const;
    bool operator!=(const BigDecimal& rhs) const;

    // Type queries
    bool isZero() const;
    bool isPositive() const;
    bool isNegative() const;
    bool isInteger() const;
    bool isNaN() const;
    bool isInfinite() const;

    // Conversion
    std::string toString(FormatMode mode = FormatMode::FLOATING_POINT,
                         int digits = 0, int base = 10) const;
    std::string toBaseString(int base, FormatMode mode = FormatMode::FLOATING_POINT,
                             int digits = 0) const;
    double toDouble() const;
    int64_t toInt64() const;

    // Constants
    static BigDecimal pi(int precision = DEFAULT_PRECISION);
    static BigDecimal e(int precision = DEFAULT_PRECISION);
    static BigDecimal fromBase(const std::string& value, int base,
                               int precision = DEFAULT_PRECISION);

    static int getDefaultPrecision();
    static void setDefaultPrecision(int precision);
    int getPrecision() const;

    // MPFR direct access — allows functions.cpp to use MPFR directly
    // for high-precision trig/log operations without losing precision
    struct MpfrAccess {
        const void* mpfrPtr;  ///< Pointer to underlying mpfr_t
        int precision;
    };
    MpfrAccess getMpfrAccess() const;

    /// Construct from an MPFR function result (internal use)
    static BigDecimal fromMpfrResult(std::function<void(mpfr_t)> mpfrFunc, int precision);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

inline BigDecimal operator+(const BigDecimal& a, const BigDecimal& b) { return a.add(b); }
inline BigDecimal operator-(const BigDecimal& a, const BigDecimal& b) { return a.sub(b); }
inline BigDecimal operator*(const BigDecimal& a, const BigDecimal& b) { return a.mul(b); }
inline BigDecimal operator/(const BigDecimal& a, const BigDecimal& b) { return a.div(b); }
inline BigDecimal operator%(const BigDecimal& a, const BigDecimal& b) { return a.mod(b); }
inline BigDecimal operator-(const BigDecimal& a) { return a.neg(); }

} // namespace calc

#endif // CALC_BIG_DECIMAL_H
