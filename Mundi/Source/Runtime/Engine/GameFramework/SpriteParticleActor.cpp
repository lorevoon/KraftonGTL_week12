#include "pch.h"
#include "SpriteParticleActor.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "Modules/ParticleModuleRequired.h"
#include "Modules/ParticleModuleLifetime.h"
#include "Modules/ParticleModuleSize.h"
#include "Modules/ParticleModuleColor.h"
#include "Modules/ParticleModuleVelocity.h"
#include "Modules/ParticleModuleLocation.h"
#include "ObjectFactory.h"
#include "Material.h"
#include "ResourceManager.h"

ASpriteParticleActor::ASpriteParticleActor()
{
	ObjectName = "Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// 샘플 파티클 시스템 생성
	UParticleSystem* ParticleSystem = NewObject<UParticleSystem>();
	ParticleSystem->SystemName = "Sample Particle System";
	ParticleSystem->SystemDuration = 5.0f;
	ParticleSystem->SystemLoops = 0; // 무한 반복
	ParticleSystem->bAutoActivate = true;

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->EmitterName = "Sample Emitter";
	Emitter->MaxParticleCount = 100;

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->Level = 0;
	LODLevel->DistanceThreshold = 0.0f;

	// 파티클용 머티리얼 생성
	UMaterial* ParticleMaterial = NewObject<UMaterial>();

	// ParticleSprite 쉐이더 로드
	UShader* ParticleShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleSprite.hlsl");
	ParticleMaterial->SetShader(ParticleShader);

	// 파티클 텍스처 로드
	UTexture* ParticleTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/Boom.png");

	// MaterialInfo 설정
	FMaterialInfo MaterialInfo;
	MaterialInfo.MaterialName = "ParticleMaterial";
	MaterialInfo.DiffuseTextureFileName = "Data/Textures/Boom.png";
	ParticleMaterial->SetMaterialInfo(MaterialInfo);
	ParticleMaterial->ResolveTextures();

	// Required 모듈 (필수)
	UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();
	RequiredModule->SpawnRate = 15.0f; // 초당 20개 파티클 생성
	RequiredModule->EmitterDuration = 1.0f;
	RequiredModule->EmitterLoops = 0; // 무한 반복
	RequiredModule->Material = ParticleMaterial; // 파티클 머티리얼 할당
	LODLevel->RequiredModule = RequiredModule;

	// Lifetime 모듈
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->LifetimeMin = 1.0f;
	LifetimeModule->LifetimeMax = 5.0f;
	LODLevel->AddModule(LifetimeModule);

	// Size 모듈
	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
	SizeModule->StartSizeMin = FVector(1.0f, 1.0f, 1.0f);
	SizeModule->StartSizeMax = FVector(2.0f, 2.0f, 2.0f);
	LODLevel->AddModule(SizeModule);

	// Color 모듈
	UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
	ColorModule->StartColor = FLinearColor(1.0f, 0.5f, 0.0f, 0.2f); // 주황색
	ColorModule->EndColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.8f); // 노란색
	LODLevel->AddModule(ColorModule);

	// Velocity 모듈
	UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
	VelocityModule->StartVelocityMin = FVector(3.0f, 3.0f, 3.0f);
	VelocityModule->StartVelocityMax = FVector(5.0f, 5.0f, 5.0f);
	LODLevel->AddModule(VelocityModule);

	// Location 모듈
	UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
	LocationModule->StartLocation = FVector(0.0f, 0.f, 0.0f);
	LODLevel->AddModule(LocationModule);

	// LOD 레벨을 이미터에 추가
	Emitter->AddLODLevel(LODLevel);

	// 이미터를 파티클 시스템에 추가
	ParticleSystem->AddEmitter(Emitter);

	// 파티클 시스템을 컴포넌트에 할당
	ParticleSystemComponent->Template = ParticleSystem;
	ParticleSystemComponent->bAutoActivate = true;
}

ASpriteParticleActor::~ASpriteParticleActor()
{
}

void ASpriteParticleActor::DuplicateSubObjects()
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

void ASpriteParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
