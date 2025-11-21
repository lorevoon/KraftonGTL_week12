#pragma once
#include "UEContainer.h"
#include "Name.h"

// FName에 대한 GetTypeHash 오버로드입니다.
inline uint64 GetTypeHash(const FName& Name)
{
    return static_cast<uint64>(Name.ComparisonIndex);
}

// 두 개의 해시 값을 안전하게 조합합니다. (Boost::hash_combine 알고리즘 기반)
inline uint64 HashCombine(uint64 Seed, uint64 ValueToCombine)
{
    const uint64 GoldenRatio = 0x9e3779b97f4a7c15;
    Seed ^= ValueToCombine + GoldenRatio + (Seed << 6) + (Seed >> 2);
    return Seed;
}