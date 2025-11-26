#include "pch.h"
#include "RibbonParticleActor.h"
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
#include "Modules/ParticleModuleSpawnPerUnit.h"
#include "ObjectFactory.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ParticleModuleTypeDataRibbon.h"

ARibbonParticleActor::ARibbonParticleActor()
{
	ObjectName = "Ribbon Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// 리본 파티클 시스템 생성
	UParticleSystem* ParticleSystem = NewObject<UParticleSystem>();
	ParticleSystem->SystemName = "Sample Ribbon Particle System";
	ParticleSystem->SystemDuration = 10.0f;
	ParticleSystem->SystemLoops = 0; // 무한 반복
	ParticleSystem->bAutoActivate = true;

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->EmitterName = "Sample Ribbon Emitter";
	Emitter->MaxParticleCount = 2000; // 리본 궤적을 위해 많은 파티클 필요

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->Level = 0;
	LODLevel->DistanceThreshold = 0.0f;

	// 파티클용 머티리얼 생성
	UMaterial* ParticleMaterial = NewObject<UMaterial>();

	// 빔 쉐이더 로드 (리본도 빔과 동일한 셰이더 사용)
	UShader* BeamShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleBeam.hlsl");
	ParticleMaterial->SetShader(BeamShader);

	// MaterialInfo 설정
	FMaterialInfo MaterialInfo;
	MaterialInfo.MaterialName = "RibbonParticleMaterial";
	ParticleMaterial->SetMaterialInfo(MaterialInfo);
	ParticleMaterial->ResolveTextures();

	// Required 모듈 (필수)
	UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();
	RequiredModule->SpawnRate = 0.0f; // SpawnPerUnit 모듈이 거리 기반으로 스폰하므로 0
	RequiredModule->EmitterDuration = 5.0f;
	RequiredModule->EmitterLoops = 0; // 무한 반복
	RequiredModule->Material = ParticleMaterial;
	RequiredModule->bUseLocalSpace = false;
	LODLevel->RequiredModule = RequiredModule;

	// Ribbon TypeData 모듈 (TypeDataModule로 설정)
	UParticleModuleTypeDataRibbon* RibbonModule = NewObject<UParticleModuleTypeDataRibbon>();
	RibbonModule->Width = 2.0f;                      // 리본 너비 2m
	RibbonModule->MaxParticleInTrailCount = 200;      // 최대 200개 파티클로 궤적 구성 (긴 궤적)
	RibbonModule->TilingDistance = 5.0f;             // 5m마다 UV 타일링
	RibbonModule->RenderAxis = ERibbonRenderAxis::CameraUp;  // 카메라를 향하도록
	RibbonModule->DistanceTessellationStepSize = 0.5f;       // 0.5m마다 테셀레이션
	RibbonModule->MaxTessellationBetweenParticles = 25;      // 최대 25개 보간
	RibbonModule->bEnableTangentDiffInterpScale = true;      // Tangent 각도로 추가 세분화
	LODLevel->TypeDataModule = RibbonModule;

	// SpawnPerUnit 모듈 (거리 기반 스폰)
	UParticleModuleSpawnPerUnit* SpawnPerUnitModule = NewObject<UParticleModuleSpawnPerUnit>();
	SpawnPerUnitModule->SpawnPerUnit = 20.0f;           // 단위당 20개
	SpawnPerUnitModule->UnitScalar = 1.0f;              // 1m당 (더 조밀하게)
	SpawnPerUnitModule->MaxFrameDistance = 200;         // 프레임당 최대 200개
	SpawnPerUnitModule->bSpawnOnMovementStart = true;   // 첫 이동 시 스폰
	LODLevel->AddModule(SpawnPerUnitModule);

	// Lifetime 모듈 (긴 수명으로 궤적 유지)
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->LifetimeMin = 5.0f;
	LifetimeModule->LifetimeMax = 10.0f;
	LODLevel->AddModule(LifetimeModule);

	// Size 모듈 (리본 크기)
	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
	SizeModule->StartSizeMin = FVector(1.0f, 1.0f, 1.0f);
	SizeModule->StartSizeMax = FVector(1.0f, 1.0f, 1.0f);
	LODLevel->AddModule(SizeModule);

	// Color 모듈 (리본 색상 - 밝은 색상에서 페이드)
	UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
	ColorModule->StartColor = FLinearColor(1.0f, 0.5f, 0.2f, 1.0f); // 주황색
	ColorModule->EndColor = FLinearColor(1.0f, 0.8f, 0.2f, 0.0f);   // 노란색으로 페이드
	LODLevel->AddModule(ColorModule);

	// Velocity, Location 모듈 불필요 - Trail은 컴포넌트 이동 경로에 스폰되어 정적으로 유지됨

	// LOD 레벨을 이미터에 추가
	Emitter->AddLODLevel(LODLevel);

	// 이미터를 파티클 시스템에 추가
	ParticleSystem->AddEmitter(Emitter);

	// 파티클 시스템을 컴포넌트에 할당
	ParticleSystemComponent->Template = ParticleSystem;
	ParticleSystemComponent->bAutoActivate = true;
}

ARibbonParticleActor::~ARibbonParticleActor()
{
}

void ARibbonParticleActor::DuplicateSubObjects()
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

void ARibbonParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
