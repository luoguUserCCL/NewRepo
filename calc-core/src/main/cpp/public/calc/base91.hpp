// ============================================================================
//  base91.hpp  --  basE91 decoder/encoder (Joachim Henke's algorithm)
//
//  Header-only C++17 implementation. Used by the sci-calc native toolchain to
//  safely recover credentials (e.g. a GitHub PAT) and to embed encoded assets.
//
//  Alphabet (EXACT, per project spec, 91 chars):
//    ABCDEFGHIJKLMNOPQRSTUVWXYZ
//    abcdefghijklmnopqrstuvwxyz
//    0123456789
//    !#$%&()*+,./:;<=>?@[]^_`{|}~"
//
//  Security:
//   - decoded bytes are kept in memory only as long as needed;
//   - call secure_zero() to wipe buffers after use;
//   - this header NEVER prints secrets to stdout/cerr by itself.
// ============================================================================
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace sci::util {

inline constexpr char kB91Alphabet[92] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!#$%&()*+,./:;<=>?@[]^_`{|}~\"";
// note: 91 printable chars + 1 NUL terminator => size 92.

struct B91IndexTable {
    std::array<int8_t, 256> v{};
    constexpr B91IndexTable() : v{} {
        for (auto& x : v) x = -1;
        for (int i = 0; i < 91; ++i) {
            v[static_cast<unsigned char>(kB91Alphabet[i])] =
                static_cast<int8_t>(i);
        }
    }
};

inline constexpr B91IndexTable kB91DecodeTable{};

// Decode Base91-encoded ASCII text into raw bytes.
inline std::vector<uint8_t> b91_decode(std::string_view in) {
    std::vector<uint8_t> out;
    out.reserve(in.size() * 7 / 8 + 4);

    uint32_t b = 0;   // bit accumulator
    int      n = 0;   // bit count
    int      v = -1;  // pending char value

    for (char ch : in) {
        const auto idx = kB91DecodeTable.v[static_cast<unsigned char>(ch)];
        if (idx < 0) continue;  // skip unknown / whitespace
        if (v < 0) {
            v = idx;
        } else {
            v += idx * 91;
            b |= static_cast<uint32_t>(v) << n;
            n += (v & 8191) > 88 ? 13 : 14;
            while (true) {
                out.push_back(static_cast<uint8_t>(b & 0xFF));
                b >>= 8;
                n -= 8;
                if (!(n > 7)) break;
            }
            v = -1;
        }
    }
    if (v != -1) {
        out.push_back(static_cast<uint8_t>((b | (static_cast<uint32_t>(v) << n)) & 0xFF));
    }
    return out;
}

// Encode raw bytes to a Base91 string (round-trip helper).
inline std::string b91_encode(const uint8_t* data, std::size_t len) {
    std::string out;
    out.reserve(len * 10 / 8 + 2);
    uint32_t b = 0;
    int      n = 0;
    for (std::size_t i = 0; i < len; ++i) {
        b |= static_cast<uint32_t>(data[i]) << n;
        n += 8;
        if (n > 13) {
            uint32_t v = b & 8191;
            if (v > 88) { b >>= 13; n -= 13; }
            else        { v = b & 16383; b >>= 14; n -= 14; }
            out.push_back(kB91Alphabet[v % 91]);
            out.push_back(kB91Alphabet[v / 91]);
        }
    }
    if (n) {
        out.push_back(kB91Alphabet[b % 91]);
        if (n > 7 || b > 90) out.push_back(kB91Alphabet[b / 91]);
    }
    return out;
}

inline std::string b91_encode(std::string_view raw) {
    return b91_encode(reinterpret_cast<const uint8_t*>(raw.data()), raw.size());
}

// Wipe a buffer using volatile writes (thwart simple dead-store optimisation).
inline void secure_zero(void* p, std::size_t n) {
    volatile uint8_t* vp = static_cast<volatile uint8_t*>(p);
    while (n--) *vp++ = 0;
}

inline void secure_zero(std::vector<uint8_t>& v) {
    if (!v.empty()) secure_zero(v.data(), v.size());
    v.clear();
}

}  // namespace sci::util
