#pragma once

#include <unordered_map>

// bit�����w�肷��K�v������Ƃ��́A�ȉ��̌^���g�p����B
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;

// Binary�\�L
// Binary<11110>::value �Ƃ���΁A30 �ƂȂ�B
// �����Ȃ�64bit�Ȃ̂�19���܂ŕ\�L�\�B
template <u64 n> struct Binary {
	static const u64 value = n % 10 + (Binary<n / 10>::value << 1);
};
// template ���ꉻ
template <> struct Binary<0> {
	static const u64 value = 0;
};
