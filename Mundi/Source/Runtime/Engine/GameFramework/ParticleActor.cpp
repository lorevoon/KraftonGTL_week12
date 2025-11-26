#include "pch.h"
#include "ParticleActor.h"
#include "ParticleSystemComponent.h"
#include "ObjectFactory.h"

AParticleActor::AParticleActor()
{
	ObjectName = "Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// Template은 설정하지 않음 - Property Window에서 선택하여 사용
	ParticleSystemComponent->Template = nullptr;
	ParticleSystemComponent->bAutoActivate = true;
}

AParticleActor::~AParticleActor()
{
}

void AParticleActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	for (UActorComponent* Component : OwnedComponents)
	{
		if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Component))
		{
			ParticleSystemComponent = PSC;
			break;
		}
	}
}

void AParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
