#pragma once
#include "Vector.h"

// Single curve key with Hermite tangent support
struct FParticleCurveKey
{
    float Time = 0.0f;          // 0.0-1.0 normalized particle lifetime
    float Value = 0.0f;
    float ArriveTangent = 0.0f;
    float LeaveTangent = 0.0f;
    uint8 InterpMode = 0;       // 0=Auto, 1=User, 2=Break, 3=Linear, 4=Constant

    void Serialize(bool bLoad, JSON& Json);
};

// A single curve channel (e.g., R, G, B, or A)
struct FParticleCurveChannel
{
    TArray<FParticleCurveKey> Keys;

    // Evaluate curve at given time using Hermite interpolation
    float Evaluate(float Time) const;

    void Serialize(bool bLoad, JSON& Json);
};

// Multi-channel curve for vector/color properties
struct FParticleCurve
{
    TArray<FParticleCurveChannel> Channels;  // 1 for float, 3 for vector, 4 for color

    float EvaluateChannel(int32 Index, float Time) const;
    FLinearColor EvaluateColor(float Time) const;  // Convenience for RGBA
    FVector EvaluateVector(float Time) const;      // Convenience for XYZ

    void Serialize(bool bLoad, JSON& Json);

    // Initialize with default linear interpolation from start to end
    void InitLinear(const FLinearColor& Start, const FLinearColor& End);
    void InitLinear(const FVector& Start, const FVector& End);
    void InitLinear(float Start, float End);

    // Check if curve has any keys
    bool HasKeys() const;
};
