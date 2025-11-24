#pragma once
#include "ASpriteParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="파티클 액터", Description="파티클 시스템을 생성하는 액터입니다")
class ASpriteParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()
	
	ASpriteParticleActor();
protected:
	~ASpriteParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
