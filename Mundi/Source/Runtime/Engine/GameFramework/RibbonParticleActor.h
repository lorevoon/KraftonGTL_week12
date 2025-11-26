#pragma once
#include "ARibbonParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="리본 파티클 액터 (샘플)", Description="샘플 리본 파티클 시스템이 설정된 액터입니다")
class ARibbonParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	ARibbonParticleActor();
protected:
	~ARibbonParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
