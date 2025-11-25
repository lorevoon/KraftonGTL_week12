#pragma once
#include "ABeamParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="빔 파티클 액터", Description="빔 파티클 시스템을 생성하는 액터입니다")
class ABeamParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	ABeamParticleActor();
protected:
	~ABeamParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
