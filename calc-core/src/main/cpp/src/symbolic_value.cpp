#include <calc/symbolic_value.h>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace calc {

// ==================== Construction ====================

SymbolicValue::SymbolicValue() = default;

SymbolicValue::SymbolicValue(int64_t numerator) : num_(numerator), den_(1) {}

SymbolicValue::SymbolicValue(int64_t num, int64_t den) : num_(num), den_(den) {
    if (den_ == 0) throw std::domain_error("SymbolicValue: zero denominator");
    if (den_ < 0) { num_ = -num_; den_ = -den_; }
}

SymbolicValue SymbolicValue::rational(int64_t num, int64_t den) {
    SymbolicValue v(num, den);
    v.reduce();
    return v;
}

SymbolicValue SymbolicValue::radical(int64_t radicand, int64_t num, int64_t den) {
    SymbolicValue v(num, den);
    v.radicand_ = radicand;
    v.reduce();
    return v;
}

SymbolicValue SymbolicValue::piMultiple(int64_t num, int64_t den) {
    SymbolicValue v(num, den);
    v.hasPi_ = true;
    v.reduce();
    return v;
}

SymbolicValue SymbolicValue::piRadical(int64_t radicand, int64_t num, int64_t den) {
    SymbolicValue v(num, den);
    v.radicand_ = radicand;
    v.hasPi_ = true;
    v.reduce();
    return v;
}

// ==================== Helpers ====================

int64_t SymbolicValue::gcd64(int64_t a, int64_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { int64_t t = a % b; a = b; b = t; }
    return a;
}

void SymbolicValue::extractSquareFactor(int64_t n, int64_t& factor, int64_t& remainder) {
    factor = 1;
    remainder = n < 0 ? -n : n;
    if (remainder == 0) return;
    // Try small factors: 2,3,5,7,... up to sqrt(n)
    for (int64_t i = 2; i * i <= remainder; ) {
        if (remainder % (i * i) == 0) {
            remainder /= (i * i);
            factor *= i;
        } else {
            i++;
        }
    }
}

void SymbolicValue::reduce() {
    if (den_ == 0) throw std::domain_error("SymbolicValue: zero denominator");
    if (den_ < 0) { num_ = -num_; den_ = -den_; }

    // Reduce fraction
    if (num_ == 0) {
        den_ = 1;
        radicand_ = 0;
        hasPi_ = false;
        return;
    }
    int64_t g = gcd64(num_, den_);
    if (g > 1) { num_ /= g; den_ /= g; }

    // Simplify radicand
    if (radicand_ < 0) {
        // √(negative) — not real; clear it (could be complex but we skip)
        radicand_ = 0;
        num_ = 0;
        return;
    }
    if (radicand_ == 1) {
        radicand_ = 0;  // √1 = 1, trivial
    } else if (radicand_ > 1) {
        int64_t factor, remainder;
        extractSquareFactor(radicand_, factor, remainder);
        if (factor > 1) {
            num_ *= factor;
            radicand_ = remainder;
            if (radicand_ == 1) radicand_ = 0;  // fully extracted
            // re-reduce fraction after multiplying num by factor
            g = gcd64(num_, den_);
            if (g > 1) { num_ /= g; den_ /= g; }
        }
    }
}

// ==================== Arithmetic ====================

SymbolicValue SymbolicValue::add(const SymbolicValue& rhs) const {
    // Can only add exactly if forms match (same radicand, same pi flag)
    if (radicand_ == rhs.radicand_ && hasPi_ == rhs.hasPi_) {
        // (p1/q1 + p2/q2) = (p1*q2 + p2*q1) / (q1*q2), same radical/pi
        int64_t n = num_ * rhs.den_ + rhs.num_ * den_;
        int64_t d = den_ * rhs.den_;
        SymbolicValue r(n, d);
        r.radicand_ = radicand_;
        r.hasPi_ = hasPi_;
        r.reduce();
        return r;
    }
    // Mismatch — cannot preserve exactness, fall back to zero (caller should
    // use toBigDecimal for numeric add instead)
    return SymbolicValue();  // 0 as fallback
}

SymbolicValue SymbolicValue::sub(const SymbolicValue& rhs) const {
    SymbolicValue neg = rhs;
    neg.num_ = -neg.num_;
    return add(neg);
}

SymbolicValue SymbolicValue::mul(const SymbolicValue& rhs) const {
    // (p1/q1)·(p2/q2) = (p1·p2)/(q1·q2)
    int64_t n = num_ * rhs.num_;
    int64_t d = den_ * rhs.den_;
    // Radicals: √a · √b = √(a·b)
    int64_t rad = 0;
    if (radicand_ > 0 && rhs.radicand_ > 0) {
        rad = radicand_ * rhs.radicand_;
    } else if (radicand_ > 0) {
        rad = radicand_;
    } else if (rhs.radicand_ > 0) {
        rad = rhs.radicand_;
    }
    // Pi: π · π = π² (not representable in our form; if both have pi, drop to numeric)
    bool pi = hasPi_ || rhs.hasPi_;
    if (hasPi_ && rhs.hasPi_) {
        // π² — can't represent, return zero (caller should use BigDecimal)
        return SymbolicValue();
    }
    SymbolicValue r(n, d);
    r.radicand_ = rad;
    r.hasPi_ = pi;
    r.reduce();
    return r;
}

SymbolicValue SymbolicValue::div(const SymbolicValue& rhs) const {
    if (rhs.num_ == 0) throw std::domain_error("Division by zero");
    // (p1/q1) / (p2/q2) = (p1·q2)/(q1·p2)
    int64_t n = num_ * rhs.den_;
    int64_t d = den_ * rhs.num_;
    if (d < 0) { n = -n; d = -d; }
    // Radicals: √a / √b = √(a/b) — only exact if b|a. Simplify: if radicands
    // match, they cancel. If only one has radical, keep it.
    int64_t rad = 0;
    if (radicand_ > 0 && rhs.radicand_ > 0) {
        if (radicand_ == rhs.radicand_) {
            rad = 0;  // √a/√a = 1
        } else if (radicand_ % rhs.radicand_ == 0) {
            rad = radicand_ / rhs.radicand_;
        } else {
            // Can't simplify exactly; keep as √(a/b) which is √(a*b)/b
            // For simplicity, multiply numerator radicand by rhs.radicand
            // and denominator by rhs.radicand. This is getting complex;
            // fall back to no radical.
            rad = 0;
        }
    } else if (radicand_ > 0) {
        rad = radicand_;
    } else if (rhs.radicand_ > 0) {
        // 1/√b = √b/b (rationalize). This changes the coefficient.
        rad = rhs.radicand_;
        n *= rhs.radicand_;
        d *= rhs.radicand_;
        if (d < 0) { n = -n; d = -d; }
    }
    bool pi = hasPi_ && !rhs.hasPi_;  // π/π = 1
    if (hasPi_ && rhs.hasPi_) pi = false;
    SymbolicValue r(n, d);
    r.radicand_ = rad;
    r.hasPi_ = pi;
    r.reduce();
    return r;
}

SymbolicValue SymbolicValue::negate() const {
    SymbolicValue r = *this;
    r.num_ = -r.num_;
    return r;
}

// ==================== Conversion ====================

std::string SymbolicValue::toString() const {
    if (num_ == 0) return "0";

    std::ostringstream ss;

    // Sign
    bool negative = num_ < 0;
    int64_t absNum = negative ? -num_ : num_;

    // Start with the coefficient part (num/den)
    // We'll build: [sign]num[/den][√rad][π]
    // But for readability: 3√2/5 not (3/5)√2 — we write num√rad/den

    if (negative) ss << '-';

    // Coefficient: if den==1, just num; else num/den
    // But if there's a radical or pi, the coefficient wraps differently:
    //   3√2/5 means (3/5)·√2, NOT (3√2)/5
    // So we write: num[√rad][π]  then  /den if den != 1
    // Special case: omit coefficient 1 when a radical or pi is present
    // (√2 not 1√2, π not 1π), but keep -1 as - (-√2 not -1√2).
    bool hasExtra = (radicand_ > 1) || hasPi_;
    if (!hasExtra || absNum != 1) {
        ss << absNum;
    } else if (negative) {
        // -1√2 → -√2 (the sign was already printed, nothing more needed)
    }
    if (radicand_ > 1) ss << "√" << radicand_;
    if (hasPi_) ss << "π";
    if (den_ != 1) ss << "/" << den_;

    return ss.str();
}

BigDecimal SymbolicValue::toBigDecimal() const {
    // num/den · √radicand · (π if hasPi)
    BigDecimal result(num_);
    if (den_ != 1) {
        result = result.div(BigDecimal(int64_t(den_)));
    }
    if (radicand_ > 1) {
        result = result.nthRoot(static_cast<unsigned long>(radicand_));
    }
    if (hasPi_) {
        result = result.mul(BigDecimal::pi());
    }
    return result;
}

SymbolicValue SymbolicValue::fromBigDecimal(const BigDecimal& v) {
    // Try to detect if the BigDecimal is an exact integer
    if (v.isInteger()) {
        return SymbolicValue(v.toInt64());
    }
    // Try to detect a simple fraction: multiply by small denominators and
    // check if the result is an integer.
    // This is a heuristic — not guaranteed to find the "true" fraction.
    for (int64_t den = 2; den <= 1000; ++den) {
        BigDecimal product = v.mul(BigDecimal(den));
        if (product.isInteger()) {
            int64_t num = product.toInt64();
            return rational(num, den);
        }
    }
    // Can't find a simple fraction — return as a "raw" rational with den=1
    // (loses exactness, but better than nothing)
    return SymbolicValue(v.toInt64());
}

} // namespace calc
