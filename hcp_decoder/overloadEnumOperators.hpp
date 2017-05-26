#pragma once

#define OverloadEnumOperators(T)                                        \
    inline T& operator += (T& lhs, const int rhs) { return lhs  = static_cast<T>(static_cast<int>(lhs) + rhs); } \
    inline T& operator += (T& lhs, const T   rhs) { return lhs += static_cast<int>(rhs); } \
    inline T& operator -= (T& lhs, const int rhs) { return lhs  = static_cast<T>(static_cast<int>(lhs) - rhs); } \
    inline T& operator -= (T& lhs, const T   rhs) { return lhs -= static_cast<int>(rhs); } \
    inline T& operator *= (T& lhs, const int rhs) { return lhs  = static_cast<T>(static_cast<int>(lhs) * rhs); } \
    inline T& operator /= (T& lhs, const int rhs) { return lhs  = static_cast<T>(static_cast<int>(lhs) / rhs); } \
    inline constexpr T operator + (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) + rhs); } \
    inline constexpr T operator + (const T   lhs, const T   rhs) { return lhs + static_cast<int>(rhs); } \
    inline constexpr T operator - (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) - rhs); } \
    inline constexpr T operator - (const T   lhs, const T   rhs) { return lhs - static_cast<int>(rhs); } \
    inline constexpr T operator * (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) * rhs); } \
    inline constexpr T operator * (const int lhs, const T   rhs) { return rhs * lhs; } \
    inline constexpr T operator * (const T   lhs, const T   rhs) { return lhs * static_cast<int>(rhs); } \
    inline constexpr T operator / (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) / rhs); } \
    inline constexpr int operator / (const T   lhs, const T rhs) { return static_cast<int>(lhs) / static_cast<int>(rhs); } \
    inline constexpr T operator - (const T   rhs) { return static_cast<T>(-static_cast<int>(rhs)); } \
    inline T operator ++ (T& lhs) { lhs += 1; return lhs; } /* �O�u */  \
    inline T operator -- (T& lhs) { lhs -= 1; return lhs; } /* �O�u */  \
    inline T operator ++ (T& lhs, int) { const T temp = lhs; lhs += 1; return temp; } /* ��u */ \
	/* inline T operator -- (T& lhs, int) { const T temp = lhs; lhs -= 1; return temp; } */ /* ��u */
