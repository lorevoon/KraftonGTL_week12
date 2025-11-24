#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"

UParticleSystem::UParticleSystem()
    : SystemName("ParticleSystem")
    , SystemDuration(0.0f)
    , SystemLoops(0)
    , bAutoActivate(true)
{
}

UParticleEmitter* UParticleSystem::GetEmitter(int32 Index) const
{
    if (Index >= 0 && Index < Emitters.Num())
    {
        return Emitters[Index];
    }
    return nullptr;
}

void UParticleSystem::AddEmitter(UParticleEmitter* Emitter)
{
    if (Emitter)
    {
        Emitters.Add(Emitter);
    }
}

void UParticleSystem::RemoveEmitter(UParticleEmitter* Emitter)
{
    if (Emitter)
    {
        Emitters.Remove(Emitter);
    }
}

void UParticleSystem::SwapEmitters(int32 IndexA, int32 IndexB)
{
    if (IndexA >= 0 && IndexA < Emitters.Num() && IndexB >= 0 && IndexB < Emitters.Num())
    {
        UParticleEmitter* Temp = Emitters[IndexA];
        Emitters[IndexA] = Emitters[IndexB];
        Emitters[IndexB] = Temp;
    }
}

void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Load
        FString SystemNameValue;
        float SystemDurationValue;
        int32 SystemLoopsValue;
        bool bAutoActivateValue;

        FJsonSerializer::ReadString(InOutHandle, "SystemName", SystemNameValue, "ParticleSystem", false);
        FJsonSerializer::ReadFloat(InOutHandle, "SystemDuration", SystemDurationValue, 0.0f, false);
        FJsonSerializer::ReadInt32(InOutHandle, "SystemLoops", SystemLoopsValue, 0, false);
        FJsonSerializer::ReadBool(InOutHandle, "bAutoActivate", bAutoActivateValue, true, false);

        SystemName = SystemNameValue;
        SystemDuration = SystemDurationValue;
        SystemLoops = SystemLoopsValue;
        bAutoActivate = bAutoActivateValue;

        // Load emitters array
        if (InOutHandle.hasKey("Emitters"))
        {
            const JSON& EmittersArray = InOutHandle.at("Emitters");
            if (EmittersArray.JSONType() == JSON::Class::Array)
            {
                Emitters.Empty();
                for (const JSON& EmitterJson : EmittersArray.ArrayRange())
                {
                    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
                    if (Emitter)
                    {
                        JSON EmitterJsonCopy = EmitterJson;
                        Emitter->Serialize(bInIsLoading, EmitterJsonCopy);
                        Emitters.Add(Emitter);
                    }
                }
            }
        }
    }
    else
    {
        // Save
        InOutHandle["SystemName"] = SystemName.c_str();
        InOutHandle["SystemDuration"] = static_cast<double>(SystemDuration);
        InOutHandle["SystemLoops"] = static_cast<long>(SystemLoops);
        InOutHandle["bAutoActivate"] = bAutoActivate;

        // Save emitters array
        JSON EmittersArray = JSON::Make(JSON::Class::Array);
        for (UParticleEmitter* Emitter : Emitters)
        {
            if (Emitter)
            {
                JSON EmitterJson = JSON::Make(JSON::Class::Object);
                Emitter->Serialize(bInIsLoading, EmitterJson);
                EmittersArray.append(EmitterJson);
            }
        }
        InOutHandle["Emitters"] = EmittersArray;
    }
}
