/**
 * @File Utils.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/23
 * @Brief 
 */

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <amp_short_vectors.h>

//---- Hash Combination ----
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3876.pdf
template<typename T>
void hashCombine(std::size_t& seed, const T& val)
{
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// 使用种子创建哈希值的辅助通用函数
template<typename T, typename... Types>
void hashCombine(std::size_t& seed, const T& val, const Types& ... args)
{
    hashCombine(seed, val);
    hashCombine(seed, args...);
}

inline void hashCombine(std::size_t& seed) {}

// 从参数列表中创建哈希值的通用函数
template<typename... Types>
std::size_t hashVal(const Types& ... args)
{
    std::size_t seed = 0;
    hashCombine(seed, args...);
    return seed;
}

template<typename T>
std::size_t hashAligned32(const T& v)
{
    const uint32_t size = sizeof(T) / sizeof(uint32_t);
    const auto* vBits = reinterpret_cast<const uint32_t*>(&v);
    std::size_t seed = 0;
    
    for (uint32_t i = 0u; i < size; i++) {
        hashCombine(seed, vBits[i]);
    }
    return seed;
}


// 使用对齐到 32 位的结构作为类似 std::map 的容器键时使用的通用哈希函数
// 重要：只有当结构体包含整数类型时才有效
template<typename T>
struct HashAligned32
{
    std::size_t operator()(const T& s) const { return hashAligned32(s); }
};

// 使用结构体作为 std::map 类容器键时使用的通用比较函数
// 重要：只有当结构体包含整数类型时才有效
template<typename T>
struct EqualMem
{
    bool operator()(const T& l, const T& r) const { return memcmp(&l, &r, sizeof(T)) == 0; }
};
