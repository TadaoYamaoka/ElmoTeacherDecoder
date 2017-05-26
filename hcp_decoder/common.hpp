#pragma once

#include <unordered_map>

// bit幅を指定する必要があるときは、以下の型を使用する。
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;

// Binary表記
// Binary<11110>::value とすれば、30 となる。
// 符合なし64bitなので19桁まで表記可能。
template <u64 n> struct Binary {
	static const u64 value = n % 10 + (Binary<n / 10>::value << 1);
};
// template 特殊化
template <> struct Binary<0> {
	static const u64 value = 0;
};
