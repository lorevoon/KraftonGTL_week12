#include "pch.h"
#include "ParticleCurve.h"

// Hermite cubic interpolation helper
static float CubicInterp(float P0, float T0, float P1, float T1, float Alpha)
{
    float Alpha2 = Alpha * Alpha;
    float Alpha3 = Alpha2 * Alpha;

    // Hermite basis functions
    float H00 = 2 * Alpha3 - 3 * Alpha2 + 1;
    float H10 = Alpha3 - 2 * Alpha2 + Alpha;
    float H01 = -2 * Alpha3 + 3 * Alpha2;
    float H11 = Alpha3 - Alpha2;

    return H00 * P0 + H10 * T0 + H01 * P1 + H11 * T1;
}

//-----------------------------------------------------------------------------
// FParticleCurveKey
//-----------------------------------------------------------------------------

void FParticleCurveKey::Serialize(bool bLoad, JSON& Json)
{
    if (bLoad)
    {
        if (Json.hasKey("Time")) Time = static_cast<float>(Json["Time"].ToFloat());
        if (Json.hasKey("Value")) Value = static_cast<float>(Json["Value"].ToFloat());
        if (Json.hasKey("ArriveTangent")) ArriveTangent = static_cast<float>(Json["ArriveTangent"].ToFloat());
        if (Json.hasKey("LeaveTangent")) LeaveTangent = static_cast<float>(Json["LeaveTangent"].ToFloat());
        if (Json.hasKey("InterpMode")) InterpMode = static_cast<uint8>(Json["InterpMode"].ToInt());
    }
    else
    {
        Json["Time"] = static_cast<double>(Time);
        Json["Value"] = static_cast<double>(Value);
        Json["ArriveTangent"] = static_cast<double>(ArriveTangent);
        Json["LeaveTangent"] = static_cast<double>(LeaveTangent);
        Json["InterpMode"] = static_cast<long>(InterpMode);
    }
}

//-----------------------------------------------------------------------------
// FParticleCurveChannel
//-----------------------------------------------------------------------------

float FParticleCurveChannel::Evaluate(float Time) const
{
    if (Keys.Num() == 0) return 0.0f;
    if (Keys.Num() == 1) return Keys[0].Value;

    // Clamp to first/last key if outside range
    if (Time <= Keys[0].Time) return Keys[0].Value;
    if (Time >= Keys[Keys.Num() - 1].Time) return Keys[Keys.Num() - 1].Value;

    // Find the two keys surrounding this time
    int32 KeyIdx = 0;
    for (int32 i = 0; i < Keys.Num() - 1; ++i)
    {
        if (Time >= Keys[i].Time && Time <= Keys[i + 1].Time)
        {
            KeyIdx = i;
            break;
        }
    }

    const FParticleCurveKey& Key0 = Keys[KeyIdx];
    const FParticleCurveKey& Key1 = Keys[KeyIdx + 1];

    float TimeDiff = Key1.Time - Key0.Time;
    if (TimeDiff < 0.0001f) return Key0.Value;

    float Alpha = (Time - Key0.Time) / TimeDiff;

    // Handle different interpolation modes
    switch (Key0.InterpMode)
    {
    case 3: // Linear
        return Key0.Value + (Key1.Value - Key0.Value) * Alpha;

    case 4: // Constant (step)
        return Key0.Value;

    default: // 0=Auto, 1=User, 2=Break - all use Hermite
        return CubicInterp(
            Key0.Value, Key0.LeaveTangent * TimeDiff,
            Key1.Value, Key1.ArriveTangent * TimeDiff,
            Alpha
        );
    }
}

void FParticleCurveChannel::Serialize(bool bLoad, JSON& Json)
{
    if (bLoad)
    {
        Keys.Empty();
        if (Json.hasKey("Keys") && Json["Keys"].JSONType() == JSON::Class::Array)
        {
            for (int32 i = 0; i < static_cast<int32>(Json["Keys"].size()); ++i)
            {
                FParticleCurveKey Key;
                Key.Serialize(bLoad, Json["Keys"][i]);
                Keys.Add(Key);
            }
        }
    }
    else
    {
        JSON KeysArray = JSON::Make(JSON::Class::Array);
        for (int32 i = 0; i < Keys.Num(); ++i)
        {
            JSON KeyJson = JSON::Make(JSON::Class::Object);
            Keys[i].Serialize(bLoad, KeyJson);
            KeysArray.append(KeyJson);
        }
        Json["Keys"] = KeysArray;
    }
}

//-----------------------------------------------------------------------------
// FParticleCurve
//-----------------------------------------------------------------------------

float FParticleCurve::EvaluateChannel(int32 Index, float Time) const
{
    if (Index < 0 || Index >= Channels.Num()) return 0.0f;
    return Channels[Index].Evaluate(Time);
}

FLinearColor FParticleCurve::EvaluateColor(float Time) const
{
    FLinearColor Result;
    Result.R = (Channels.Num() > 0) ? Channels[0].Evaluate(Time) : 1.0f;
    Result.G = (Channels.Num() > 1) ? Channels[1].Evaluate(Time) : 1.0f;
    Result.B = (Channels.Num() > 2) ? Channels[2].Evaluate(Time) : 1.0f;
    Result.A = (Channels.Num() > 3) ? Channels[3].Evaluate(Time) : 1.0f;
    return Result;
}

FVector FParticleCurve::EvaluateVector(float Time) const
{
    FVector Result;
    Result.X = (Channels.Num() > 0) ? Channels[0].Evaluate(Time) : 0.0f;
    Result.Y = (Channels.Num() > 1) ? Channels[1].Evaluate(Time) : 0.0f;
    Result.Z = (Channels.Num() > 2) ? Channels[2].Evaluate(Time) : 0.0f;
    return Result;
}

void FParticleCurve::Serialize(bool bLoad, JSON& Json)
{
    if (bLoad)
    {
        Channels.Empty();
        if (Json.hasKey("Channels") && Json["Channels"].JSONType() == JSON::Class::Array)
        {
            for (int32 i = 0; i < static_cast<int32>(Json["Channels"].size()); ++i)
            {
                FParticleCurveChannel Channel;
                Channel.Serialize(bLoad, Json["Channels"][i]);
                Channels.Add(Channel);
            }
        }
    }
    else
    {
        JSON ChannelsArray = JSON::Make(JSON::Class::Array);
        for (int32 i = 0; i < Channels.Num(); ++i)
        {
            JSON ChannelJson = JSON::Make(JSON::Class::Object);
            Channels[i].Serialize(bLoad, ChannelJson);
            ChannelsArray.append(ChannelJson);
        }
        Json["Channels"] = ChannelsArray;
    }
}

void FParticleCurve::InitLinear(const FLinearColor& Start, const FLinearColor& End)
{
    Channels.Empty();

    // R channel
    FParticleCurveChannel ChannelR;
    FParticleCurveKey KeyR0, KeyR1;
    KeyR0.Time = 0.0f; KeyR0.Value = Start.R; KeyR0.InterpMode = 3; // Linear
    KeyR1.Time = 1.0f; KeyR1.Value = End.R; KeyR1.InterpMode = 3;
    ChannelR.Keys.Add(KeyR0);
    ChannelR.Keys.Add(KeyR1);
    Channels.Add(ChannelR);

    // G channel
    FParticleCurveChannel ChannelG;
    FParticleCurveKey KeyG0, KeyG1;
    KeyG0.Time = 0.0f; KeyG0.Value = Start.G; KeyG0.InterpMode = 3;
    KeyG1.Time = 1.0f; KeyG1.Value = End.G; KeyG1.InterpMode = 3;
    ChannelG.Keys.Add(KeyG0);
    ChannelG.Keys.Add(KeyG1);
    Channels.Add(ChannelG);

    // B channel
    FParticleCurveChannel ChannelB;
    FParticleCurveKey KeyB0, KeyB1;
    KeyB0.Time = 0.0f; KeyB0.Value = Start.B; KeyB0.InterpMode = 3;
    KeyB1.Time = 1.0f; KeyB1.Value = End.B; KeyB1.InterpMode = 3;
    ChannelB.Keys.Add(KeyB0);
    ChannelB.Keys.Add(KeyB1);
    Channels.Add(ChannelB);

    // A channel
    FParticleCurveChannel ChannelA;
    FParticleCurveKey KeyA0, KeyA1;
    KeyA0.Time = 0.0f; KeyA0.Value = Start.A; KeyA0.InterpMode = 3;
    KeyA1.Time = 1.0f; KeyA1.Value = End.A; KeyA1.InterpMode = 3;
    ChannelA.Keys.Add(KeyA0);
    ChannelA.Keys.Add(KeyA1);
    Channels.Add(ChannelA);
}

void FParticleCurve::InitLinear(const FVector& Start, const FVector& End)
{
    Channels.Empty();

    // X channel
    FParticleCurveChannel ChannelX;
    FParticleCurveKey KeyX0, KeyX1;
    KeyX0.Time = 0.0f; KeyX0.Value = Start.X; KeyX0.InterpMode = 3;
    KeyX1.Time = 1.0f; KeyX1.Value = End.X; KeyX1.InterpMode = 3;
    ChannelX.Keys.Add(KeyX0);
    ChannelX.Keys.Add(KeyX1);
    Channels.Add(ChannelX);

    // Y channel
    FParticleCurveChannel ChannelY;
    FParticleCurveKey KeyY0, KeyY1;
    KeyY0.Time = 0.0f; KeyY0.Value = Start.Y; KeyY0.InterpMode = 3;
    KeyY1.Time = 1.0f; KeyY1.Value = End.Y; KeyY1.InterpMode = 3;
    ChannelY.Keys.Add(KeyY0);
    ChannelY.Keys.Add(KeyY1);
    Channels.Add(ChannelY);

    // Z channel
    FParticleCurveChannel ChannelZ;
    FParticleCurveKey KeyZ0, KeyZ1;
    KeyZ0.Time = 0.0f; KeyZ0.Value = Start.Z; KeyZ0.InterpMode = 3;
    KeyZ1.Time = 1.0f; KeyZ1.Value = End.Z; KeyZ1.InterpMode = 3;
    ChannelZ.Keys.Add(KeyZ0);
    ChannelZ.Keys.Add(KeyZ1);
    Channels.Add(ChannelZ);
}

void FParticleCurve::InitLinear(float Start, float End)
{
    Channels.Empty();

    FParticleCurveChannel Channel;
    FParticleCurveKey Key0, Key1;
    Key0.Time = 0.0f; Key0.Value = Start; Key0.InterpMode = 3;
    Key1.Time = 1.0f; Key1.Value = End; Key1.InterpMode = 3;
    Channel.Keys.Add(Key0);
    Channel.Keys.Add(Key1);
    Channels.Add(Channel);
}

bool FParticleCurve::HasKeys() const
{
    for (const FParticleCurveChannel& Channel : Channels)
    {
        if (Channel.Keys.Num() > 0) return true;
    }
    return false;
}
