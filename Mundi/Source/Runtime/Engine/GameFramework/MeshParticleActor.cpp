#include "pch.h"
#include "MeshParticleActor.h"
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
#include "Modules/TypeData/ParticleModuleTypeDataMesh.h"
#include "ObjectFactory.h"
#include "Material.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

AMeshParticleActor::AMeshParticleActor()
{
	ObjectName = "Mesh Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	RootComponent = ParticleSystemComponent;

	// 샘플 파티클 시스템 생성
	UParticleSystem* ParticleSystem = NewObject<UParticleSystem>();
	ParticleSystem->SystemName = "Sample Mesh Particle System";
	ParticleSystem->SystemDuration = 5.0f;
	ParticleSystem->SystemLoops = 0; // 무한 반복
	ParticleSystem->bAutoActivate = true;

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->EmitterName = "Sample Mesh Emitter";
	Emitter->MaxParticleCount = 50;

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->Level = 0;
	LODLevel->DistanceThreshold = 0.0f;

	// 파티클용 머티리얼 생성
	UMaterial* ParticleMaterial = NewObject<UMaterial>();

	// BasicPBR 쉐이더 로드
	UShader* ParticleShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleMesh.hlsl");
	ParticleMaterial->SetShader(ParticleShader);

	// 기본 텍스처 로드
	UTexture* DefaultTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/Default.png");

	// MaterialInfo 설정
	FMaterialInfo MaterialInfo;
	MaterialInfo.MaterialName = "MeshParticleMaterial";
	MaterialInfo.DiffuseTextureFileName = "Data/Textures/Default.png";
	ParticleMaterial->SetMaterialInfo(MaterialInfo);
	ParticleMaterial->ResolveTextures();

	// Required 모듈 (필수)
	UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();
	RequiredModule->SpawnRate = 5.0f; // 초당 10개 파티클 생성
	RequiredModule->EmitterDuration = 1.0f;
	RequiredModule->EmitterLoops = 0; // 무한 반복
	RequiredModule->Material = ParticleMaterial;
	LODLevel->RequiredModule = RequiredModule;

	// TypeData 모듈 (메시)
	UParticleModuleTypeDataMesh* TypeDataMesh = NewObject<UParticleModuleTypeDataMesh>();

	// 큐브 메시 로드
	UStaticMesh* CubeMesh = UResourceManager::GetInstance().Load<UStaticMesh>("Data/cube-tex.obj");
	TypeDataMesh->Mesh = CubeMesh;

	LODLevel->TypeDataModule = TypeDataMesh;

	// Lifetime 모듈
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->LifetimeMin = 2.0f;
	LifetimeModule->LifetimeMax = 4.0f;
	LODLevel->AddModule(LifetimeModule);

	// Size 모듈
	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
	SizeModule->StartSizeMin = FVector(0.5f, 0.5f, 0.5f);
	SizeModule->StartSizeMax = FVector(1.0f, 1.0f, 1.0f);
	LODLevel->AddModule(SizeModule);

	// Color 모듈
	UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
	ColorModule->StartColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
	ColorModule->EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // 흰색
	LODLevel->AddModule(ColorModule);

	// Velocity 모듈
	UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
	VelocityModule->StartVelocityMin = FVector(-5.0f, -5.0f, 2.0f);
	VelocityModule->StartVelocityMax = FVector(5.0f, 5.0f, 8.0f);
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

AMeshParticleActor::~AMeshParticleActor()
{
}

void AMeshParticleActor::DuplicateSubObjects()
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

void AMeshParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
