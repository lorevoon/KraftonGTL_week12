// ────────────────────────────────────────────────────────────────────────────
// Pawn.cpp
// Pawn 클래스 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "Pawn.h"
#include "Controller.h"
#include "InputComponent.h"
#include "InputManager.h"

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

APawn::APawn()
	: Controller(nullptr)
	, InputComponent(nullptr)
	, PendingMovementInput(FVector())
	, bNormalizeMovementInput(true)
	, MovementSpeed(25.0f)        // 에디터 기본 속도: 10.0f * 2.5f = 25.0f
	, RotationSensitivity(0.05f)  // 에디터 마우스 감도: 0.05f
	, CurrentPitch(0.0f)
	, MinPitch(-89.0f)            // 거의 아래를 볼 수 있도록
	, MaxPitch(89.0f)             // 거의 위를 볼 수 있도록
{
	// InputComponent 생성
	InputComponent = CreateDefaultSubobject<UInputComponent>("InputComponent");
}

APawn::~APawn()
{
	// 컴포넌트는 AActor::DestroyAllComponents()에서 정리됨
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void APawn::BeginPlay()
{
	Super::BeginPlay();
}

void APawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

// ────────────────────────────────────────────────────────────────────────────
// Controller 관련
// ────────────────────────────────────────────────────────────────────────────

void APawn::PossessedBy(AController* NewController)
{
	Controller = NewController;
}

void APawn::UnPossessed()
{
	Controller = nullptr;
}

// ────────────────────────────────────────────────────────────────────────────
// 입력 관련
// ────────────────────────────────────────────────────────────────────────────

void APawn::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	// 기본 APawn은 입력 바인딩을 하지 않음
	// Character, DefaultPawn 등 파생 클래스에서 필요한 입력만 바인딩

	// 에디터 카메라 스타일로 동작하는 기본 Pawn이 필요한 경우:
	// - World Settings의 Default Pawn Class를 APawn으로 설정하면
	// - Tick()의 마우스 회전 + WASD 이동 로직이 자동으로 작동함
	// - 입력 바인딩은 Tick()에서 InputManager를 직접 사용하므로 불필요
}

// ────────────────────────────────────────────────────────────────────────────
// 이동 입력 처리
// ────────────────────────────────────────────────────────────────────────────

void APawn::AddMovementInput(FVector WorldDirection, float ScaleValue)
{
	// 입력이 유효하지 않으면 무시
	if (ScaleValue == 0.0f || WorldDirection.SizeSquared() == 0.0f)
	{
		return;
	}

	// 방향 정규화
	FVector NormalizedDirection = WorldDirection.GetNormalized();

	// 스케일 적용하여 누적
	PendingMovementInput += NormalizedDirection * ScaleValue;
}

FVector APawn::ConsumeMovementInput()
{
	FVector Result = PendingMovementInput;

	// 정규화 옵션이 켜져 있고, 입력이 있으면 정규화
	if (bNormalizeMovementInput && Result.SizeSquared() > 0.0f)
	{
		Result = Result.GetNormalized();
	}

	// 입력 초기화
	PendingMovementInput = FVector();

	return Result;
}

void APawn::AddControllerYawInput(float DeltaYaw)
{
	if (DeltaYaw != 0.0f)
	{
		// Yaw는 월드 Z축 기준으로 회전 (좌우 회전)
		DeltaYaw *= RotationSensitivity;

		// 현재 회전에 Yaw 추가
		FQuat CurrentRotation = GetActorRotation();
		FVector EulerAngles = CurrentRotation.ToEulerZYXDeg();

		EulerAngles.Z += DeltaYaw; // Yaw

		// Yaw를 0~360 범위로 정규화
		while (EulerAngles.Z >= 360.0f) EulerAngles.Z -= 360.0f;
		while (EulerAngles.Z < 0.0f) EulerAngles.Z += 360.0f;

		// Pitch 유지하면서 Yaw만 업데이트
		EulerAngles.Y = CurrentPitch; // Pitch
		EulerAngles.X = 0.0f;         // Roll (사용 안 함)

		FQuat NewRotation = FQuat::MakeFromEulerZYX(EulerAngles);
		SetActorRotation(NewRotation);
	}
}

void APawn::AddControllerPitchInput(float DeltaPitch)
{
	if (DeltaPitch != 0.0f)
	{
		// Pitch는 로컬 Y축 기준으로 회전 (상하 회전)
		DeltaPitch *= RotationSensitivity;

		// CurrentPitch에 추가
		CurrentPitch += DeltaPitch;

		// Pitch 제한
		CurrentPitch = FMath::Clamp(CurrentPitch, MinPitch, MaxPitch);

		// 현재 Yaw 유지하면서 Pitch 업데이트
		FQuat CurrentRotation = GetActorRotation();
		FVector EulerAngles = CurrentRotation.ToEulerZYXDeg();

		EulerAngles.Y = CurrentPitch; // Pitch
		EulerAngles.X = 0.0f;         // Roll (사용 안 함)
		// EulerAngles.Z는 현재 Yaw 유지

		FQuat NewRotation = FQuat::MakeFromEulerZYX(EulerAngles);
		SetActorRotation(NewRotation);
	}
}

void APawn::AddControllerRollInput(float DeltaRoll)
{
	// 기본 구현: 아무것도 하지 않음
	// 비행기 등에서 필요 시 오버라이드
}

// ────────────────────────────────────────────────────────────────────────────
// 복제
// ────────────────────────────────────────────────────────────────────────────

void APawn::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// 컴포넌트 참조 재설정
	for (UActorComponent* Component : OwnedComponents)
	{
		if (UInputComponent* Input = Cast<UInputComponent>(Component))
		{
			InputComponent = Input;
		}
	}
}

void APawn::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		for (UActorComponent* Component : OwnedComponents)
		{
			if (UInputComponent* Input = Cast<UInputComponent>(Component))
			{
				InputComponent = Input;
				break;
			}
		}
	}
}
