#include "pch.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"

UParticleEmitter::UParticleEmitter()
    : EmitterName("ParticleEmitter")
    , MaxParticleCount(100)
{
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODIndex) const
{
    if (LODIndex >= 0 && LODIndex < LODLevels.Num())
    {
        return LODLevels[LODIndex];
    }
    return nullptr;
}

void UParticleEmitter::AddLODLevel(UParticleLODLevel* LODLevel)
{
    if (LODLevel)
    {
        LODLevels.Add(LODLevel);
    }
}
