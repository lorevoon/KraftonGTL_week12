#pragma once
#include "AParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="파티클 액터", Description="빈 파티클 시스템 컴포넌트를 가진 액터입니다. Template을 설정하여 사용합니다.")
class AParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AParticleActor();
protected:
	~AParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
