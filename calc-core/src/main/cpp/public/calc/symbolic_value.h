#ifndef CALC_SYMBOLIC_VALUE_H
#define CALC_SYMBOLIC_VALUE_H

#include <string>
#include <optional>
#include <calc/big_decimal.h>

namespace calc {

/**
 * Output mode for the calculator engine.
 * - DECIMAL: always reduce to a BigDecimal (floating/fixed/scientific).
 * - MATH: preserve exact symbolic forms (fractions, radicals, π, e).
 */
enum class OutputMode {
    DECIMAL,   ///< 10/4 → 2.5
    MATH       ///< 10/4 → 5/2, sqrt(2) → √2, 2*pi → 2π
};

/**
 * Represents an exact symbolic mathematical value for "math output" mode.
 *
 * Forms supported:
 *   - Rational:  p/q           (integer or fraction, gcd-reduced)
 *   - Pi-multiple: (p/q)·π     (e.g. 3π/2)
 *   - Radical:  (p/q)·√n       (e.g. 3√2/5, n square-free)
 *   - Pi-radical: (p/q)·√n·π   (combination)
 *
 * All forms store a rational coefficient (p/q) plus optional radical radicand n
 * and optional pi flag. This covers the common cases the spec requires:
 * fractions, √, and π preservation.
 */
class SymbolicValue {
public:
    /// Default: zero
    SymbolicValue();

    /// Integer constructor (p/1)
    explicit SymbolicValue(int64_t numerator);

    /// Rational constructor
    SymbolicValue(int64_t num, int64_t den);

    // Accessors
    int64_t numerator() const { return num_; }
    int64_t denominator() const { return den_; }
    int64_t radicand() const { return radicand_; }   ///< 0 = no radical, 1 = perfect square
    bool hasPi() const { return hasPi_; }
    bool isInteger() const { return den_ == 1 && radicand_ <= 1 && !hasPi_; }

    // Builders
    static SymbolicValue rational(int64_t num, int64_t den);
    static SymbolicValue radical(int64_t radicand, int64_t num, int64_t den);
    static SymbolicValue piMultiple(int64_t num, int64_t den);
    static SymbolicValue piRadical(int64_t radicand, int64_t num, int64_t den);

    // Arithmetic (symbolic — preserves exactness when possible)
    SymbolicValue add(const SymbolicValue& rhs) const;
    SymbolicValue sub(const SymbolicValue& rhs) const;
    SymbolicValue mul(const SymbolicValue& rhs) const;
    SymbolicValue div(const SymbolicValue& rhs) const;
    SymbolicValue negate() const;

    /// Convert to a human-readable exact string.
    /// Examples: "5/2", "3π", "√2", "3√2/5", "2π/3"
    std::string toString() const;

    /// Convert to a BigDecimal (lossy — for decimal mode or when caller needs numeric).
    BigDecimal toBigDecimal() const;

    /// Parse a BigDecimal into a SymbolicValue (tries to detect rational).
    static SymbolicValue fromBigDecimal(const BigDecimal& v);

    /// Reduce: gcd-reduce the fraction, simplify the radicand (extract square factors).
    void reduce();

    /// Check if the symbolic value is exactly zero.
    bool isZero() const { return num_ == 0; }

private:
    int64_t num_ = 0;       ///< numerator
    int64_t den_ = 1;       ///< denominator (always > 0 after reduce)
    int64_t radicand_ = 0;  ///< 0 = no radical; 1 = trivial (integer); >1 = √radicand
    bool hasPi_ = false;    ///< multiply by π

    /// Helper: compute gcd
    static int64_t gcd64(int64_t a, int64_t b);
    /// Helper: extract square factor from n. Returns (factor, remainder) so
    /// that n = factor^2 * remainder, remainder is square-free.
    static void extractSquareFactor(int64_t n, int64_t& factor, int64_t& remainder);
};

} // namespace calc

#endif // CALC_SYMBOLIC_VALUE_H
