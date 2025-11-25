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

void UParticleEmitter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Load LOD levels array
        if (InOutHandle.hasKey("LODLevels"))
        {
            const JSON& LODLevelsArray = InOutHandle.at("LODLevels");
            if (LODLevelsArray.JSONType() == JSON::Class::Array)
            {
                LODLevels.Empty();
                for (const JSON& LODJson : LODLevelsArray.ArrayRange())
                {
                    UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();
                    if (LOD)
                    {
                        JSON LODJsonCopy = LODJson;
                        LOD->Serialize(bInIsLoading, LODJsonCopy);
                        LODLevels.Add(LOD);
                    }
                }
            }
        }
    }
    else
    {
        // Save LOD levels array
        JSON LODLevelsArray = JSON::Make(JSON::Class::Array);
        for (UParticleLODLevel* LOD : LODLevels)
        {
            if (LOD)
            {
                JSON LODJson = JSON::Make(JSON::Class::Object);
                LOD->Serialize(bInIsLoading, LODJson);
                LODLevelsArray.append(LODJson);
            }
        }
        InOutHandle["LODLevels"] = LODLevelsArray;
    }
}
