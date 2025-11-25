#include "pch.h"
#include "ParticleCollisionQuery.h"
#include "World.h"
#include "WorldPartitionManager.h"
#include "AABB.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "Actor.h"
#include "PrimitiveComponent.h"

namespace ParticleCollision
{
	bool LineTraceSingle(
		UWorld* World,
		const FVector& Start,
		const FVector& End,
		FParticleHitResult& OutHit)
	{
		OutHit.Reset();

		if (!World)
		{
			return false;
		}

		UWorldPartitionManager* Partition = World->GetPartitionManager();
		if (!Partition)
		{
			return false;
		}

		// 레이 방향 및 최대 거리 계산
		FVector Direction = End - Start;
		float MaxDistance = Direction.Size();

		if (MaxDistance < KINDA_SMALL_NUMBER)
		{
			return false;
		}

		Direction = Direction / MaxDistance; // 정규화

		// FRay 구성
		FRay Ray;
		Ray.Origin = Start;
		Ray.Direction = Direction;

		// WorldPartitionManager를 통한 레이 쿼리
		AActor* HitActor = nullptr;
		float HitT = 0.0f;

		Partition->RayQueryClosest(Ray, HitActor, HitT);

		// 유효한 히트인지 확인 (범위 내)
		if (HitActor && HitT > 0.0f && HitT <= MaxDistance)
		{
			OutHit.bHit = true;
			OutHit.HitActor = HitActor;
			OutHit.Distance = HitT;
			OutHit.ImpactPoint = Start + Direction * HitT;

			// 히트 컴포넌트 및 노말 계산
			// RootComponent가 PrimitiveComponent인 경우 AABB를 통해 노말 계산
			if (USceneComponent* RootComp = HitActor->GetRootComponent())
			{
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(RootComp))
				{
					OutHit.HitComponent = PrimComp;

					// AABB 기반 노말 계산
					FAABB WorldAABB = PrimComp->GetWorldAABB();
					OutHit.ImpactNormal = CalculateAABBNormal(WorldAABB, OutHit.ImpactPoint);
				}
			}

			return true;
		}

		return false;
	}

	FVector CalculateAABBNormal(const FAABB& Box, const FVector& ImpactPoint)
	{
		// 충돌점에서 가장 가까운 면의 노말 계산
		// 각 축에 대해 Min/Max 면과의 거리를 비교하여 가장 가까운 면 선택

		FVector Center = Box.GetCenter();
		FVector HalfExtent = Box.GetHalfExtent();

		if (HalfExtent.X < KINDA_SMALL_NUMBER &&
			HalfExtent.Y < KINDA_SMALL_NUMBER &&
			HalfExtent.Z < KINDA_SMALL_NUMBER)
		{
			return FVector(0.0f, 0.0f, 1.0f);
		}

		// 충돌점의 로컬 좌표 (박스 중심 기준)
		FVector LocalPoint = ImpactPoint - Center;

		// 각 축에 대해 정규화된 위치 계산 (-1 ~ 1 범위)
		float NormX = (HalfExtent.X > KINDA_SMALL_NUMBER) ? (LocalPoint.X / HalfExtent.X) : 0.0f;
		float NormY = (HalfExtent.Y > KINDA_SMALL_NUMBER) ? (LocalPoint.Y / HalfExtent.Y) : 0.0f;
		float NormZ = (HalfExtent.Z > KINDA_SMALL_NUMBER) ? (LocalPoint.Z / HalfExtent.Z) : 0.0f;

		// 절대값이 가장 큰 축이 충돌 면
		float AbsX = std::abs(NormX);
		float AbsY = std::abs(NormY);
		float AbsZ = std::abs(NormZ);

		if (AbsX >= AbsY && AbsX >= AbsZ)
		{
			// X축 면
			return (NormX >= 0.0f) ? FVector(1.0f, 0.0f, 0.0f) : FVector(-1.0f, 0.0f, 0.0f);
		}
		else if (AbsY >= AbsX && AbsY >= AbsZ)
		{
			// Y축 면
			return (NormY >= 0.0f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, -1.0f, 0.0f);
		}
		else
		{
			// Z축 면
			return (NormZ >= 0.0f) ? FVector(0.0f, 0.0f, 1.0f) : FVector(0.0f, 0.0f, -1.0f);
		}
	}

	FVector CalculateOBBNormal(const FOBB& Box, const FVector& ImpactPoint)
	{
		// OBB의 로컬 공간에서 계산 후 월드 공간으로 변환
		FVector LocalPoint = ImpactPoint - Box.Center;

		// 각 축에 대한 투영
		float ProjX = FVector::Dot(LocalPoint, Box.Axes[0]);
		float ProjY = FVector::Dot(LocalPoint, Box.Axes[1]);
		float ProjZ = FVector::Dot(LocalPoint, Box.Axes[2]);

		// 정규화
		float NormX = (Box.HalfExtent.X > KINDA_SMALL_NUMBER) ? (ProjX / Box.HalfExtent.X) : 0.0f;
		float NormY = (Box.HalfExtent.Y > KINDA_SMALL_NUMBER) ? (ProjY / Box.HalfExtent.Y) : 0.0f;
		float NormZ = (Box.HalfExtent.Z > KINDA_SMALL_NUMBER) ? (ProjZ / Box.HalfExtent.Z) : 0.0f;

		float AbsX = std::abs(NormX);
		float AbsY = std::abs(NormY);
		float AbsZ = std::abs(NormZ);

		if (AbsX >= AbsY && AbsX >= AbsZ)
		{
			return (NormX >= 0.0f) ? Box.Axes[0] : (Box.Axes[0] * -1.0f);
		}
		else if (AbsY >= AbsX && AbsY >= AbsZ)
		{
			return (NormY >= 0.0f) ? Box.Axes[1] : (Box.Axes[1] * -1.0f);
		}
		else
		{
			return (NormZ >= 0.0f) ? Box.Axes[2] : (Box.Axes[2] * -1.0f);
		}
	}

	FVector CalculateSphereNormal(const FBoundingSphere& Sphere, const FVector& ImpactPoint)
	{
		// 구의 경우: 중심에서 충돌점 방향이 노말
		FVector ToImpact = ImpactPoint - Sphere.Center;
		float Length = ToImpact.Size();

		if (Length < KINDA_SMALL_NUMBER)
		{
			return FVector(0.0f, 0.0f, 1.0f);
		}

		return ToImpact / Length;
	}
}
