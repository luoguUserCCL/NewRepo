#include <calc/crypto_random.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#ifdef PLATFORM_LINUX
#include <fstream>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <bcrypt.h>
#endif

namespace calc {

bool CryptoRandom::randomBytes(void* buffer, size_t size) {
    if (!buffer || size == 0) return true;

#ifdef PLATFORM_LINUX
    // Use /dev/urandom (cryptographically secure on modern Linux)
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom.is_open()) return false;
    urandom.read(static_cast<char*>(buffer), size);
    return urandom.gcount() == static_cast<std::streamsize>(size);
#endif

#ifdef PLATFORM_WINDOWS
    NTSTATUS status = BCryptGenRandom(
        nullptr,
        static_cast<PUCHAR>(buffer),
        static_cast<ULONG>(size),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    return status == 0;
#endif

#ifdef PLATFORM_MACOS
    // macOS: use arc4random_buf (cryptographically secure)
    arc4random_buf(buffer, size);
    return true;
#endif

    // Fallback: use std::rand (NOT cryptographically secure)
    unsigned char* bytes = static_cast<unsigned char*>(buffer);
    for (size_t i = 0; i < size; i++) {
        bytes[i] = static_cast<unsigned char>(std::rand() & 0xFF);
    }
    return true;
}

uint64_t CryptoRandom::randomUInt64() {
    uint64_t result;
    randomBytes(&result, sizeof(result));
    return result;
}

BigDecimal CryptoRandom::uniform(const BigDecimal& minimum,
                                   const BigDecimal& maximum,
                                   int precision) {
    // Generate a random number in [0, 1) with full precision
    // Then scale to [minimum, maximum]

    // Number of random bytes needed for the given precision
    int bits = precision;
    int bytes = (bits + 7) / 8;

    // Generate random bytes and construct a fraction [0, 1)
    std::vector<unsigned char> randBytes(bytes);
    if (!randomBytes(randBytes.data(), bytes)) {
        throw std::runtime_error("Failed to generate cryptographically secure random bytes");
    }

    // Convert random bytes to a BigDecimal in [0, 1)
    // fraction = sum(randBytes[i] * 256^(bytes-1-i)) / 256^bytes
    BigDecimal fraction(int64_t(0));
    BigDecimal scale(int64_t(1));
    for (int i = bytes - 1; i >= 0; i--) {
        fraction = fraction.add(BigDecimal(static_cast<int64_t>(randBytes[i])).mul(scale));
        scale = scale.mul(BigDecimal(int64_t(256)));
    }
    fraction = fraction.div(scale);

    // Scale to [minimum, maximum]
    // result = minimum + fraction * (maximum - minimum)
    BigDecimal range = maximum.sub(minimum);
    return minimum.add(fraction.mul(range));
}

bool CryptoRandom::isAvailable() {
    unsigned char test;
    return randomBytes(&test, 1);
}

} // namespace calc
