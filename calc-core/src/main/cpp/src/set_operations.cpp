#include <calc/set_operations.h>
#include <algorithm>

namespace calc {

// ==================== CalcSet Construction ====================

CalcSet CalcSet::makeEnumerated(std::vector<BigDecimal> elements) {
    CalcSet s;
    s.data_ = Enumerated{std::move(elements)};
    return s;
}

CalcSet CalcSet::makeInterval(BoundType left, BoundType right,
                                BigDecimal low, BigDecimal high) {
    CalcSet s;
    s.data_ = Interval{std::move(low), std::move(high), left, right};
    return s;
}

CalcSet CalcSet::makeComputed(SetOp op, CalcSet left, CalcSet right) {
    CalcSet s;
    s.data_ = Computed{op,
        std::make_unique<CalcSet>(std::move(left)),
        std::make_unique<CalcSet>(std::move(right))};
    return s;
}

// ==================== CalcSet Operations ====================

bool CalcSet::contains(const BigDecimal& value) const {
    return std::visit([&](const auto& d) -> bool {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, Enumerated>) {
            for (const auto& elem : d.elements) {
                if (elem == value) return true;
            }
            return false;
        } else if constexpr (std::is_same_v<T, Interval>) {
            if (d.leftBound == BoundType::CLOSED) {
                if (value < d.low) return false;
            } else {
                if (value <= d.low) return false;
            }
            if (d.rightBound == BoundType::CLOSED) {
                if (value > d.high) return false;
            } else {
                if (value >= d.high) return false;
            }
            return true;
        } else if constexpr (std::is_same_v<T, Computed>) {
            if (d.op == SetOp::UNION) {
                return d.left->contains(value) || d.right->contains(value);
            } else {
                return d.left->contains(value) && d.right->contains(value);
            }
        }
        return false;
    }, data_);
}

CalcSet CalcSet::intersection(const CalcSet& other) const {
    return makeComputed(SetOp::INTERSECTION, clone(), other.clone());
}

CalcSet CalcSet::union_(const CalcSet& other) const {
    return makeComputed(SetOp::UNION, clone(), other.clone());
}

CalcSet CalcSet::clone() const {
    return std::visit([](const auto& d) -> CalcSet {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, Enumerated>) {
            return makeEnumerated(std::vector<BigDecimal>(d.elements));
        } else if constexpr (std::is_same_v<T, Interval>) {
            return makeInterval(d.leftBound, d.rightBound, d.low, d.high);
        } else if constexpr (std::is_same_v<T, Computed>) {
            return makeComputed(d.op, d.left->clone(), d.right->clone());
        }
        return makeEnumerated({});
    }, data_);
}

std::string CalcSet::toString() const {
    return std::visit([](const auto& d) -> std::string {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, Enumerated>) {
            std::string s = "{";
            for (size_t i = 0; i < d.elements.size(); i++) {
                if (i > 0) s += ", ";
                s += d.elements[i].toString();
            }
            s += "}";
            return s;
        } else if constexpr (std::is_same_v<T, Interval>) {
            std::string s;
            s += (d.leftBound == BoundType::CLOSED ? "[" : "(");
            s += d.low.toString() + ", " + d.high.toString();
            s += (d.rightBound == BoundType::CLOSED ? "]" : ")");
            return s;
        } else if constexpr (std::is_same_v<T, Computed>) {
            std::string opStr = (d.op == SetOp::UNION) ? " cup " : " cap ";
            return d.left->toString() + opStr + d.right->toString();
        }
        return "{}";
    }, data_);
}

// ==================== IversonEvaluator ====================

BigDecimal IversonEvaluator::evaluateComparison(BinaryOp op,
                                                   const BigDecimal& left,
                                                   const BigDecimal& right) {
    bool result = false;
    switch (op) {
        case BinaryOp::EQ:  result = (left == right); break;
        case BinaryOp::NEQ: result = (left != right); break;
        case BinaryOp::LT:  result = (left < right); break;
        case BinaryOp::GT:  result = (left > right); break;
        case BinaryOp::LEQ: result = (left <= right); break;
        case BinaryOp::GEQ: result = (left >= right); break;
        default: break;
    }
    return fromBool(result);
}

BigDecimal IversonEvaluator::evaluateMembership(const BigDecimal& value, const CalcSet& set) {
    return fromBool(set.contains(value));
}

BigDecimal IversonEvaluator::evaluateAnd(const BigDecimal& left, const BigDecimal& right) {
    return fromBool(toBool(left) && toBool(right));
}

BigDecimal IversonEvaluator::evaluateOr(const BigDecimal& left, const BigDecimal& right) {
    return fromBool(toBool(left) || toBool(right));
}

BigDecimal IversonEvaluator::evaluateNot(const BigDecimal& value) {
    return fromBool(!toBool(value));
}

bool IversonEvaluator::toBool(const BigDecimal& value) {
    return !value.isZero();
}

BigDecimal IversonEvaluator::fromBool(bool value) {
    return BigDecimal(value ? 1 : 0);
}

} // namespace calc
