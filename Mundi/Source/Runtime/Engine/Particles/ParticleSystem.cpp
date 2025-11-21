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
