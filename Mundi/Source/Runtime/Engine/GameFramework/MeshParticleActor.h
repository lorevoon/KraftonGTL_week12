#pragma once
#include "AMeshParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="메시 파티클 액터 (샘플)", Description="샘플 메시 파티클 시스템이 설정된 액터입니다")
class AMeshParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AMeshParticleActor();
protected:
	~AMeshParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
