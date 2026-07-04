#ifndef CALC_SET_OPERATIONS_H
#define CALC_SET_OPERATIONS_H

#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <calc/big_decimal.h>

namespace calc {

/**
 * Represents a mathematical set in the calculator.
 * Supports enumerated sets, intervals, and computed sets (union/intersection).
 */
class CalcSet {
public:
    /**
     * Interval boundary type.
     */
    enum class BoundType {
        CLOSED,   ///< [ or ]
        OPEN      ///< ( or )
    };

    /**
     * Closed interval [low, high]
     */
    struct Interval {
        BigDecimal low;
        BigDecimal high;
        BoundType leftBound;
        BoundType rightBound;
    };

    /**
     * Enumerated set {a, b, c, ...}
     */
    struct Enumerated {
        std::vector<BigDecimal> elements;
    };

    /**
     * Computed set from set operations (union/intersection).
     * Stores operands for lazy evaluation.
     */
    enum class SetOp { UNION, INTERSECTION };

    struct Computed {
        SetOp op;
        std::unique_ptr<CalcSet> left;
        std::unique_ptr<CalcSet> right;
    };

    using Data = std::variant<Enumerated, Interval, Computed>;

    // ==================== Construction ====================

    /// Create an enumerated set {elements...}
    static CalcSet makeEnumerated(std::vector<BigDecimal> elements);

    /// Create an interval
    static CalcSet makeInterval(BoundType left, BoundType right,
                                 BigDecimal low, BigDecimal high);

    /// Create a computed set (union or intersection)
    static CalcSet makeComputed(SetOp op, CalcSet left, CalcSet right);

    // ==================== Operations ====================

    /// Check if a value belongs to this set
    bool contains(const BigDecimal& value) const;

    /// Set intersection (cap → ∩)
    CalcSet intersection(const CalcSet& other) const;

    /// Set union (cup → ∪)
    CalcSet union_(const CalcSet& other) const;

    /// Debug string representation
    std::string toString() const;

    /// Get the underlying data variant
    const Data& data() const { return data_; }

private:
    Data data_;
};

/**
 * Evaluate an Iverson bracket predicate.
 * Returns 1 if the predicate is true, 0 otherwise.
 *
 * Supports:
 *   Comparison: =, <, >, ≤, ≥, ≠
 *   Logic: and (∧), or (∨), not (¬)
 *   Set membership: in (∈)
 */
class IversonEvaluator {
public:
    /// Evaluate a comparison predicate
    static BigDecimal evaluateComparison(BinaryOp op,
                                          const BigDecimal& left,
                                          const BigDecimal& right);

    /// Evaluate set membership
    static BigDecimal evaluateMembership(const BigDecimal& value, const CalcSet& set);

    /// Evaluate logical AND
    static BigDecimal evaluateAnd(const BigDecimal& left, const BigDecimal& right);

    /// Evaluate logical OR
    static BigDecimal evaluateOr(const BigDecimal& left, const BigDecimal& right);

    /// Evaluate logical NOT
    static BigDecimal evaluateNot(const BigDecimal& value);

    /// Convert BigDecimal to boolean (0 = false, nonzero = true)
    static bool toBool(const BigDecimal& value);

    /// Convert boolean to BigDecimal (true = 1, false = 0)
    static BigDecimal fromBool(bool value);
};

} // namespace calc

#endif // CALC_SET_OPERATIONS_H
