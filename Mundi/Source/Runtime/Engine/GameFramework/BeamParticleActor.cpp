#include "pch.h"
#include "BeamParticleActor.h"
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
#include "Modules/ParticleModuleSpawn.h"
#include "Modules/TypeData/ParticleModuleTypeDataBeam.h"
#include "ObjectFactory.h"
#include "Material.h"
#include "ResourceManager.h"

ABeamParticleActor::ABeamParticleActor()
{
	ObjectName = "Beam Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// 빔 파티클 시스템 생성
	UParticleSystem* ParticleSystem = NewObject<UParticleSystem>();
	ParticleSystem->SystemName = "Sample Beam Particle System";
	ParticleSystem->SystemDuration = 5.0f;
	ParticleSystem->SystemLoops = 0; // 무한 반복
	ParticleSystem->bAutoActivate = true;

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->EmitterName = "Sample Beam Emitter";
	Emitter->MaxParticleCount = 100;

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->Level = 0;
	LODLevel->DistanceThreshold = 0.0f;

	// 파티클용 머티리얼 생성
	UMaterial* ParticleMaterial = NewObject<UMaterial>();

	// 빔 쉐이더 로드
	UShader* BeamShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleBeam.hlsl");
	ParticleMaterial->SetShader(BeamShader);

	// 빔 텍스처 로드 (기본: 흰색 그라데이션 또는 기본 텍스처)
	UTexture* BeamTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/Default.png");

	// MaterialInfo 설정
	FMaterialInfo MaterialInfo;
	MaterialInfo.MaterialName = "BeamParticleMaterial";
	MaterialInfo.DiffuseTextureFileName = "Data/Textures/Default.png";
	ParticleMaterial->SetMaterialInfo(MaterialInfo);
	ParticleMaterial->ResolveTextures();

	// Required 모듈 (필수)
	UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();
	RequiredModule->SpawnRate = 10.0f; // 초당 5개 빔 생성
	RequiredModule->EmitterDuration = 5.0f;
	RequiredModule->EmitterLoops = 0; // 무한 반복
	RequiredModule->Material = ParticleMaterial;
	LODLevel->RequiredModule = RequiredModule;

	// TypeData 모듈 (빔)
	UParticleModuleTypeDataBeam* TypeDataBeam = NewObject<UParticleModuleTypeDataBeam>();
	TypeDataBeam->BeamMethod = EBeamMethod::Distance;
	TypeDataBeam->Segments = 5;        // 세그먼트 수 (노이즈 표현용)
	TypeDataBeam->Width = 1.5f;         // 빔 폭
	TypeDataBeam->Length = 50.0f;      // 빔 길이
	TypeDataBeam->NoiseStrength = 1.5f; // 노이즈 강도 (번개 효과)
	TypeDataBeam->NoiseFrequency = 2.5f; // 노이즈 주파수
	LODLevel->TypeDataModule = TypeDataBeam;

	// Spawn 모듈
	UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
	SpawnModule->LocationMin = FVector(0.0f, 0.0f, 0.0f);
	SpawnModule->LocationMax = FVector(0.0f, 0.0f, 0.0f);
	SpawnModule->DistributionType = EDistributionType::Constant;
	LODLevel->AddModule(SpawnModule);

	// Lifetime 모듈
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->LifetimeMin = 0.5f;
	LifetimeModule->LifetimeMax = 1.0f;
	LODLevel->AddModule(LifetimeModule);

	// Size 모듈 (빔 폭 스케일에 사용)
	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
	SizeModule->StartSizeMin = FVector(1.0f, 1.0f, 1.0f);
	SizeModule->StartSizeMax = FVector(1.0f, 1.0f, 1.0f);
	LODLevel->AddModule(SizeModule);

	// Color 모듈 (빔 색상)
	UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
	ColorModule->StartColor = FLinearColor(0.5f, 0.8f, 1.0f, 1.0f); // 하늘색
	ColorModule->EndColor = FLinearColor(0.2f, 0.5f, 1.0f, 0.0f);   // 페이드 아웃
	LODLevel->AddModule(ColorModule);

	// Velocity 모듈 (빔 방향 결정)
	UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
	VelocityModule->StartVelocityMin = FVector(1.0f, 0.0f, 0.0f);  // X 방향
	VelocityModule->StartVelocityMax = FVector(1.0f, 0.0f, 0.0f);
	LODLevel->AddModule(VelocityModule);

	// Location 모듈
	UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
	LocationModule->StartLocation = FVector(0.0f, 0.0f, 0.0f);
	LODLevel->AddModule(LocationModule);

	// LOD 레벨을 이미터에 추가
	Emitter->AddLODLevel(LODLevel);

	// 이미터를 파티클 시스템에 추가
	ParticleSystem->AddEmitter(Emitter);

	// 파티클 시스템을 컴포넌트에 할당
	ParticleSystemComponent->Template = ParticleSystem;
	ParticleSystemComponent->bAutoActivate = true;
}

ABeamParticleActor::~ABeamParticleActor()
{
}

void ABeamParticleActor::DuplicateSubObjects()
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

void ABeamParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
