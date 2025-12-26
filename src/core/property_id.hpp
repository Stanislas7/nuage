#pragma once
#include <cstddef>

namespace nuage {

using PropertyId = size_t;

// FNV-1a 64-bit hash
constexpr PropertyId hashString(const char* str, size_t n) {
    PropertyId hash = 14695981039346656037ULL;
    for (size_t i = 0; i < n; ++i) {
        hash ^= static_cast<unsigned char>(str[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

constexpr PropertyId hashString(const char* str) {
    size_t n = 0;
    while (str[n] != '\0') ++n;
    return hashString(str, n);
}

// User-defined literal for convenience: "my/prop"_id
constexpr PropertyId operator"" _id(const char* str, size_t n) {
    return hashString(str, n);
}

/**
 * @brief A property key that carries its type information for safe and clean API usage.
 */
template<typename T>
struct TypedProperty {
    PropertyId id;

    constexpr TypedProperty(const char* name) : id(hashString(name)) {}

    constexpr operator PropertyId() const { return id; }
};

}
