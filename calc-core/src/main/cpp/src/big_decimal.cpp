#include <calc/big_decimal.h>
#include <mpfr.h>
#include <gmp.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <mutex>

namespace calc {

// Global default precision (thread-safe)
static std::atomic<int> g_defaultPrecision{BigDecimal::DEFAULT_PRECISION};

int BigDecimal::getDefaultPrecision() {
    return g_defaultPrecision.load();
}

void BigDecimal::setDefaultPrecision(int precision) {
    if (precision < 2) precision = 2;
    g_defaultPrecision.store(precision);
}

// ======================== Implementation ========================

struct BigDecimal::Impl {
    mpfr_t value;
    int precision;

    Impl(int prec = DEFAULT_PRECISION) : precision(prec) {
        mpfr_init2(value, prec);
        mpfr_set_zero(value, 1);
    }

    Impl(const Impl& other) : precision(other.precision) {
        mpfr_init2(value, precision);
        mpfr_set(value, other.value, MPFR_RNDN);
    }

    ~Impl() {
        mpfr_clear(value);
    }
};

// Convert RoundingMode to MPFR rounding mode
static mpfr_rnd_t toMpfrRounding(RoundingMode rm) {
    switch (rm) {
        case RoundingMode::ROUND_TO_NEAREST:   return MPFR_RNDN;
        case RoundingMode::ROUND_TOWARD_ZERO:  return MPFR_RNDZ;
        case RoundingMode::ROUND_TOWARD_POS_INF: return MPFR_RNDU;
        case RoundingMode::ROUND_TOWARD_NEG_INF: return MPFR_RNDD;
        case RoundingMode::ROUND_AWAY_FROM_ZERO: return MPFR_RNDN; // MPFR doesn't have this, use nearest
        default: return MPFR_RNDN;
    }
}

// ==================== Construction ====================

BigDecimal::BigDecimal() : impl_(std::make_unique<Impl>(g_defaultPrecision.load())) {}

BigDecimal::BigDecimal(int precision) : impl_(std::make_unique<Impl>(precision)) {}

BigDecimal::BigDecimal(const std::string& value, int precision)
    : impl_(std::make_unique<Impl>(precision)) {
    mpfr_set_str(impl_->value, value.c_str(), 10, MPFR_RNDN);
}

BigDecimal::BigDecimal(double value, int precision)
    : impl_(std::make_unique<Impl>(precision)) {
    mpfr_set_d(impl_->value, value, MPFR_RNDN);
}

BigDecimal::BigDecimal(int64_t value, int precision)
    : impl_(std::make_unique<Impl>(precision)) {
    mpfr_set_sj(impl_->value, value, MPFR_RNDN);
}

BigDecimal::BigDecimal(const BigDecimal& other)
    : impl_(std::make_unique<Impl>(other.impl_->precision)) {
    mpfr_set(impl_->value, other.impl_->value, MPFR_RNDN);
}

BigDecimal::BigDecimal(BigDecimal&& other) noexcept = default;

BigDecimal::~BigDecimal() = default;

// ==================== Assignment ====================

BigDecimal& BigDecimal::operator=(const BigDecimal& other) {
    if (this != &other) {
        impl_ = std::make_unique<Impl>(*other.impl_);
    }
    return *this;
}

BigDecimal& BigDecimal::operator=(BigDecimal&& other) noexcept = default;

BigDecimal& BigDecimal::operator=(const std::string& value) {
    mpfr_set_str(impl_->value, value.c_str(), 10, MPFR_RNDN);
    return *this;
}

// ==================== Arithmetic ====================

BigDecimal BigDecimal::add(const BigDecimal& rhs, RoundingMode rm) const {
    BigDecimal result(std::max(impl_->precision, rhs.impl_->precision));
    mpfr_add(result.impl_->value, impl_->value, rhs.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::sub(const BigDecimal& rhs, RoundingMode rm) const {
    BigDecimal result(std::max(impl_->precision, rhs.impl_->precision));
    mpfr_sub(result.impl_->value, impl_->value, rhs.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::mul(const BigDecimal& rhs, RoundingMode rm) const {
    BigDecimal result(std::max(impl_->precision, rhs.impl_->precision));
    mpfr_mul(result.impl_->value, impl_->value, rhs.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::div(const BigDecimal& rhs, RoundingMode rm) const {
    if (rhs.isZero()) {
        throw std::domain_error("Division by zero");
    }
    BigDecimal result(std::max(impl_->precision, rhs.impl_->precision));
    mpfr_div(result.impl_->value, impl_->value, rhs.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::mod(const BigDecimal& rhs, RoundingMode rm) const {
    if (rhs.isZero()) {
        throw std::domain_error("Modulo by zero");
    }
    BigDecimal result(std::max(impl_->precision, rhs.impl_->precision));
    // fmod: result = this - trunc(this/rhs) * rhs
    mpfr_fmod(result.impl_->value, impl_->value, rhs.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::neg() const {
    BigDecimal result(impl_->precision);
    mpfr_neg(result.impl_->value, impl_->value, MPFR_RNDN);
    return result;
}

BigDecimal BigDecimal::abs() const {
    BigDecimal result(impl_->precision);
    mpfr_abs(result.impl_->value, impl_->value, MPFR_RNDN);
    return result;
}

BigDecimal BigDecimal::floor() const {
    BigDecimal result(impl_->precision);
    mpfr_floor(result.impl_->value, impl_->value);
    return result;
}

BigDecimal BigDecimal::ceil() const {
    BigDecimal result(impl_->precision);
    mpfr_ceil(result.impl_->value, impl_->value);
    return result;
}

BigDecimal BigDecimal::pow(int64_t exponent, RoundingMode rm) const {
    BigDecimal result(impl_->precision);
    // Fast exponentiation for integer exponents
    if (exponent >= 0) {
        // Use MPFR's optimized integer power
        mpfr_pow_sj(result.impl_->value, impl_->value, exponent, toMpfrRounding(rm));
    } else {
        // Negative exponent: 1 / this^|exp|
        mpfr_pow_sj(result.impl_->value, impl_->value, -exponent, toMpfrRounding(rm));
        mpfr_ui_div(result.impl_->value, 1, result.impl_->value, toMpfrRounding(rm));
    }
    return result;
}

BigDecimal BigDecimal::pow(const BigDecimal& exponent, RoundingMode rm) const {
    // Check if exponent is integer — use fast path
    if (exponent.isInteger()) {
        int64_t expInt = exponent.toInt64();
        return this->pow(expInt, rm);
    }
    // General case: this^exp = exp(exp * ln(this))
    BigDecimal result(std::max(impl_->precision, exponent.impl_->precision));
    if (isZero()) {
        mpfr_set_zero(result.impl_->value, 1);
        return result;
    }
    // Compute ln(this)
    BigDecimal lnThis(impl_->precision);
    mpfr_log(lnThis.impl_->value, impl_->value, toMpfrRounding(rm));
    // Compute exp * ln(this)
    BigDecimal product = exponent.mul(lnThis, rm);
    // Compute exp(product)
    mpfr_exp(result.impl_->value, product.impl_->value, toMpfrRounding(rm));
    return result;
}

BigDecimal BigDecimal::nthRoot(unsigned long n, RoundingMode rm) const {
    if (n == 0) {
        throw std::domain_error("Zero-th root is undefined");
    }
    BigDecimal result(impl_->precision);
    mpfr_rootn_ui(result.impl_->value, impl_->value, n, toMpfrRounding(rm));
    return result;
}

// ==================== Comparison ====================

int BigDecimal::compareTo(const BigDecimal& rhs) const {
    return mpfr_cmp(impl_->value, rhs.impl_->value);
}

bool BigDecimal::operator<(const BigDecimal& rhs) const { return compareTo(rhs) < 0; }
bool BigDecimal::operator<=(const BigDecimal& rhs) const { return compareTo(rhs) <= 0; }
bool BigDecimal::operator>(const BigDecimal& rhs) const { return compareTo(rhs) > 0; }
bool BigDecimal::operator>=(const BigDecimal& rhs) const { return compareTo(rhs) >= 0; }
bool BigDecimal::operator==(const BigDecimal& rhs) const { return compareTo(rhs) == 0; }
bool BigDecimal::operator!=(const BigDecimal& rhs) const { return compareTo(rhs) != 0; }

// ==================== Type queries ====================

bool BigDecimal::isZero() const { return mpfr_zero_p(impl_->value) != 0; }
bool BigDecimal::isPositive() const { return mpfr_sgn(impl_->value) > 0; }
bool BigDecimal::isNegative() const { return mpfr_sgn(impl_->value) < 0; }
bool BigDecimal::isInteger() const { return mpfr_integer_p(impl_->value) != 0; }
bool BigDecimal::isNaN() const { return mpfr_nan_p(impl_->value) != 0; }
bool BigDecimal::isInfinite() const { return mpfr_inf_p(impl_->value) != 0; }

// ==================== Conversion ====================

std::string BigDecimal::toString(FormatMode mode, int digits, int base) const {
    if (isNaN()) return "NaN";
    if (isInfinite()) return isNegative() ? "-Inf" : "Inf";
    if (isZero()) return "0";

    mpfr_exp_t exp;
    char* str = nullptr;

    switch (mode) {
        case FormatMode::SCIENTIFIC: {
            int d = digits > 0 ? digits : impl_->precision / 3;
            str = mpfr_get_str(nullptr, &exp, base, d, impl_->value, MPFR_RNDN);
            std::string s(str);
            mpfr_free_str(str);
            // Format: d.dddde+ee
            if (s.length() > 1) s.insert(1, 1, '.');
            if (exp - 1 != 0) {
                s += "e";
                if (exp - 1 > 0) s += "+";
                s += std::to_string(exp - 1);
            }
            return s;
        }
        case FormatMode::FIXED_POINT: {
            int d = digits > 0 ? digits : impl_->precision / 3;
            str = mpfr_get_str(nullptr, &exp, base, -d, impl_->value, MPFR_RNDN);
            std::string s(str);
            mpfr_free_str(str);
            return s;
        }
        case FormatMode::SIGNIFICANT_DIGITS: {
            int d = digits > 0 ? digits : impl_->precision / 3;
            str = mpfr_get_str(nullptr, &exp, base, d, impl_->value, MPFR_RNDN);
            std::string s(str);
            mpfr_free_str(str);
            if (s.length() > 1) s.insert(1, 1, '.');
            if (exp - 1 != 0 && static_cast<size_t>(exp) != s.length()) {
                s += "e";
                if (exp - 1 > 0) s += "+";
                s += std::to_string(exp - 1);
            }
            return s;
        }
        case FormatMode::FLOATING_POINT:
        default: {
            // Auto-select: use fixed if exponent is reasonable, else scientific
            int d = digits > 0 ? digits : impl_->precision / 3;
            str = mpfr_get_str(nullptr, &exp, base, d, impl_->value, MPFR_RNDN);
            std::string s(str);
            mpfr_free_str(str);
            // If exponent is in reasonable range, use fixed-like format
            if (exp > -4 && exp < d + 1) {
                // Insert decimal point at the right position
                if (static_cast<size_t>(exp) <= s.length()) {
                    if (exp > 0) {
                        s.insert(exp, 1, '.');
                        // Remove trailing zeros
                        while (s.back() == '0') s.pop_back();
                        if (s.back() == '.') s.pop_back();
                    }
                }
                return s;
            }
            // Otherwise use scientific
            if (s.length() > 1) s.insert(1, 1, '.');
            s += "e";
            if (exp - 1 > 0) s += "+";
            s += std::to_string(exp - 1);
            return s;
        }
    }
}

std::string BigDecimal::toBaseString(int base, FormatMode mode, int digits) const {
    return toString(mode, digits, base);
}

double BigDecimal::toDouble() const {
    return mpfr_get_d(impl_->value, MPFR_RNDN);
}

int64_t BigDecimal::toInt64() const {
    return mpfr_get_sj(impl_->value, MPFR_RNDZ);
}

// ==================== Static utilities ====================

BigDecimal BigDecimal::pi(int precision) {
    BigDecimal result(precision);
    mpfr_const_pi(result.impl_->value, MPFR_RNDN);
    return result;
}

BigDecimal BigDecimal::e(int precision) {
    BigDecimal one(precision);
    mpfr_set_ui(one.impl_->value, 1, MPFR_RNDN);
    BigDecimal result(precision);
    mpfr_exp(result.impl_->value, one.impl_->value, MPFR_RNDN);
    return result;
}

BigDecimal BigDecimal::fromBase(const std::string& value, int base, int precision) {
    BigDecimal result(precision);
    int b = (base >= 2 && base <= 16) ? base : 10;
    mpfr_set_str(result.impl_->value, value.c_str(), b, MPFR_RNDN);
    return result;
}

int BigDecimal::getPrecision() const {
    return impl_->precision;
}

// ==================== MPFR Direct Access ====================

BigDecimal::MpfrAccess BigDecimal::getMpfrAccess() const {
    return MpfrAccess{static_cast<const void*>(&impl_->value), impl_->precision};
}

BigDecimal BigDecimal::fromMpfrResult(std::function<void(mpfr_t)> mpfrFunc, int precision) {
    BigDecimal result(precision);
    mpfrFunc(result.impl_->value);
    return result;
}

} // namespace calc
