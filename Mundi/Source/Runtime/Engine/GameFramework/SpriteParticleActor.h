#pragma once
#include "ASpriteParticleActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="스프라이트 파티클 액터 (샘플)", Description="샘플 스프라이트 파티클 시스템이 설정된 액터입니다")
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
