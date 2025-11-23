#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataMesh.generated.h"

// 전방 선언
class UStaticMesh;

// Mesh 파티클 TypeData
// - 메시 파티클에 사용할 StaticMesh 지정
UCLASS(DisplayName="메시 타입 데이터", Description="메시 파티클에 사용할 스태틱 메시를 지정합니다")
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleTypeDataMesh() : Mesh(nullptr) {}

    // 메시 파티클에 사용할 스태틱 메시
    UPROPERTY(EditAnywhere, Category="Mesh")
    UStaticMesh* Mesh;
};
