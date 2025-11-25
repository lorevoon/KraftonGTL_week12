#include "pch.h"
#include "CollisionParticleActor.h"
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
#include "Modules/ParticleModuleAcceleration.h"
#include "Modules/ParticleModuleCollision.h"
#include "Modules/TypeData/ParticleModuleTypeDataMesh.h"
#include "ObjectFactory.h"
#include "Material.h"
#include "ResourceManager.h"
#include "StaticMesh.h"
#include "GlobalConsole.h"

ACollisionParticleActor::ACollisionParticleActor()
{
	ObjectName = "Collision Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// 파티클 시스템 생성
	UParticleSystem* ParticleSystem = NewObject<UParticleSystem>();
	ParticleSystem->SystemName = "Collision Test Particle System";
	ParticleSystem->SystemDuration = 10.0f;
	ParticleSystem->SystemLoops = 0; // 무한 반복
	ParticleSystem->bAutoActivate = true;

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->EmitterName = "Collision Test Emitter";
	Emitter->MaxParticleCount = 100;

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->Level = 0;
	LODLevel->DistanceThreshold = 0.0f;

	// 파티클용 머티리얼 생성
	UMaterial* ParticleMaterial = NewObject<UMaterial>();

	// 메시 파티클 쉐이더 로드
	UShader* ParticleShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleMesh.hlsl");
	ParticleMaterial->SetShader(ParticleShader);

	// MaterialInfo 설정
	FMaterialInfo MaterialInfo;
	MaterialInfo.MaterialName = "CollisionTestMaterial";
	MaterialInfo.DiffuseTextureFileName = "Data/Textures/Default.png";
	ParticleMaterial->SetMaterialInfo(MaterialInfo);
	ParticleMaterial->ResolveTextures();

	// Required 모듈 (필수)
	UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();
	RequiredModule->SpawnRate = 3.0f; // 초당 3개 (충돌 관찰용으로 적게)
	RequiredModule->EmitterDuration = 10.0f;
	RequiredModule->EmitterLoops = 0; // 무한 반복
	RequiredModule->Material = ParticleMaterial;
	LODLevel->RequiredModule = RequiredModule;

	// TypeData 모듈 (메시)
	UParticleModuleTypeDataMesh* TypeDataMesh = NewObject<UParticleModuleTypeDataMesh>();
	UStaticMesh* SphereMesh = UResourceManager::GetInstance().Load<UStaticMesh>("Data/apple_mid.obj");
	TypeDataMesh->Mesh = SphereMesh;
	LODLevel->TypeDataModule = TypeDataMesh;

	// Spawn 모듈
	UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
	SpawnModule->LocationMin = FVector(0.0f, 0.0f, 0.0f);
	SpawnModule->LocationMax = FVector(0.0f, 0.0f, 0.0f);
	SpawnModule->DistributionType = EDistributionType::Constant;
	LODLevel->AddModule(SpawnModule);

	// Lifetime 모듈 - 긴 수명으로 충돌 관찰
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->LifetimeMin = 10.0f;
	LifetimeModule->LifetimeMax = 100.0f;
	LODLevel->AddModule(LifetimeModule);

	// Size 모듈 - 관찰하기 좋은 크기
	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
	SizeModule->StartSizeMin = FVector(0.5f, 0.5f, 0.5f);
	SizeModule->StartSizeMax = FVector(1.0f, 1.0f, 1.0f);
	LODLevel->AddModule(SizeModule);

	// Color 모듈 - 빨간색 (충돌 테스트임을 표시)
	UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
	ColorModule->StartColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
	ColorModule->EndColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
	LODLevel->AddModule(ColorModule);

	// Velocity 모듈 - 위로 살짝 던지기 + 수평 방향 랜덤
	UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
	VelocityModule->StartVelocityMin = FVector(-5.0f, -5.0f, 10.0f);
	VelocityModule->StartVelocityMax = FVector(5.0f, 5.0f, 20.0f);
	LODLevel->AddModule(VelocityModule);

	// Location 모듈 - 높은 곳에서 시작
	UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
	LocationModule->StartLocation = FVector(0.0f, 0.0f, 30.0f); // 높이 30m에서 시작
	LODLevel->AddModule(LocationModule);

	// ========== 가속도 모듈 (중력) ==========
	UParticleModuleAcceleration* AccelModule = NewObject<UParticleModuleAcceleration>();
	AccelModule->Acceleration = FVector(0.0f, 0.0f, -9.8f); // 중력 (m/s²)
	LODLevel->AddModule(AccelModule);

	// ========== 충돌 모듈 ==========
	UParticleModuleCollision* CollisionModule = NewObject<UParticleModuleCollision>();
	CollisionModule->CollisionResponse = EParticleCollisionResponse::Bounce; // 반사
	CollisionModule->Resilience = 0.7f;   // 반사 계수 (70% 에너지 보존)
	CollisionModule->Friction = 0.1f;     // 마찰
	CollisionModule->bCollisionEnabled = true;
	CollisionModule->MaxCollisions = 5;   // 5번 충돌 후 파티클 제거
	LODLevel->AddModule(CollisionModule);

	// LOD 레벨을 이미터에 추가
	Emitter->AddLODLevel(LODLevel);

	// 이미터를 파티클 시스템에 추가
	ParticleSystem->AddEmitter(Emitter);

	// 파티클 시스템을 컴포넌트에 할당
	ParticleSystemComponent->Template = ParticleSystem;
	ParticleSystemComponent->bAutoActivate = true;

	// ========== 충돌 이벤트 콜백 등록 (디버그 로그) ==========
	ParticleSystemComponent->OnParticleCollide.Add(
		[](const FString& EventName,
		   float EmitterTime,
		   float ParticleTime,
		   const FVector& Location,
		   const FVector& Velocity,
		   const FVector& Direction,
		   const FVector& Normal,
		   const FString& BoneName)
		{
			UE_LOG("[Collision] Event=%s, Pos=(%.1f, %.1f, %.1f), Normal=(%.2f, %.2f, %.2f)",
				EventName.c_str(),
				Location.X, Location.Y, Location.Z,
				Normal.X, Normal.Y, Normal.Z);
		}
	);
}

ACollisionParticleActor::~ACollisionParticleActor()
{
}

void ACollisionParticleActor::DuplicateSubObjects()
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

void ACollisionParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
