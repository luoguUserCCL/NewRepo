#ifndef CALC_CRYPTO_RANDOM_H
#define CALC_CRYPTO_RANDOM_H

#include <calc/big_decimal.h>

namespace calc {

/**
 * Cryptographically secure random number generator.
 * Used by the rand(min, max) function.
 * Uses OS-provided CSPRNG (/dev/urandom on Linux, BCryptGenRandom on Windows).
 * Produces uniformly distributed random BigDecimals.
 */
class CryptoRandom {
public:
    /// Generate a cryptographically secure random BigDecimal
    /// in the range [minimum, maximum] with continuous uniform distribution.
    /// @param minimum Lower bound (inclusive)
    /// @param maximum Upper bound (inclusive)
    /// @param precision Bit precision for the random value
    /// @return Random BigDecimal in [minimum, maximum]
    static BigDecimal uniform(const BigDecimal& minimum,
                               const BigDecimal& maximum,
                               int precision = BigDecimal::DEFAULT_PRECISION);

    /// Fill a buffer with cryptographically secure random bytes
    /// @param buffer Output buffer
    /// @param size Number of bytes to generate
    /// @return true on success, false on failure
    static bool randomBytes(void* buffer, size_t size);

    /// Generate a random integer in [0, n)
    static uint64_t randomUInt64();

    /// Seed check — verify that the CSPRNG is available
    static bool isAvailable();
};

} // namespace calc

#endif // CALC_CRYPTO_RANDOM_H
