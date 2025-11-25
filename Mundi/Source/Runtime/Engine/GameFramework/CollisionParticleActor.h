#pragma once
#include "ACollisionParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="충돌 파티클 액터", Description="파티클 충돌 테스트용 액터입니다")
class ACollisionParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	ACollisionParticleActor();
protected:
	~ACollisionParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
