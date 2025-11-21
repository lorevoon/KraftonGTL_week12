#pragma once
#include "AnimationAsset.h"
#include "UAnimSequenceBase.generated.h"

/**
 * 애니메이션 시퀀스 베이스 클래스
 * UAnimSequence의 부모 클래스로, 시퀀스의 기본 정보를 저장
 * 실제 애니메이션 데이터는 파생 클래스에서 관리
 */
UCLASS(DisplayName="애니메이션 시퀀스 베이스", Description="애니메이션 시퀀스의 추상 베이스 클래스")
class UAnimSequenceBase : public UAnimationAsset
{
	GENERATED_REFLECTION_BODY()

public:
	UAnimSequenceBase() = default;
	virtual ~UAnimSequenceBase() = default;

	/**
	 * 애니메이션의 총 재생 길이를 반환 (초 단위)
	 * @return 재생 길이 (초)
	 */
	virtual float GetPlayLength() const override { return SequenceLength; }

	/**
	 * 시퀀스 길이 설정
	 * @param InLength 새로운 시퀀스 길이 (초)
	 */
	void SetSequenceLength(float InLength) { SequenceLength = InLength; }
	float GetSequenceLength() { return SequenceLength; }

	/**
	 * 스켈레톤 이름 가져오기
	 * @return 스켈레톤 이름
	 */
	const FString& GetSkeletonName() const { return SkeletonName; }

	/**
	 * 스켈레톤 이름 설정
	 * @param InName 새로운 스켈레톤 이름
	 */
	void SetSkeletonName(const FString& InName) { SkeletonName = InName; }

	/**
	 * Get skeleton signature for compatibility checking
	 * @return 64-bit skeleton structure hash
	 */
	uint64 GetSkeletonSignature() const { return SkeletonSignature; }

	/**
	 * Set skeleton signature when animation is loaded
	 * @param InSignature Computed skeleton signature
	 */
	void SetSkeletonSignature(uint64 InSignature) { SkeletonSignature = InSignature; }

	/**
	 * Get bone count from target skeleton
	 * @return Number of bones
	 */
	int32 GetSkeletonBoneCount() const { return SkeletonBoneCount; }

	/**
	 * Set skeleton bone count
	 * @param InBoneCount Number of bones in skeleton
	 */
	void SetSkeletonBoneCount(int32 InBoneCount) { SkeletonBoneCount = InBoneCount; }

	/**
	 * 스켈레톤 기준 로컬 포즈를 추출합니다.
	 * @param Skeleton 평가할 스켈레톤
	 * @param Time 시간(초)
	 * @param bLooping 루프 여부
	 * @param bInterpolate 키 보간 사용 여부
	 * @param OutLocalPose 본 개수 크기의 로컬 포즈 배열(출력)
	 **/
	virtual void ExtractBonePose(const FSkeleton& Skeleton, float Time, bool bLooping, bool bInterpolate, TArray<FTransform>& OutLocalPose) const;

protected:
	/** 애니메이션 전체 재생 길이 (초 단위) */
	float SequenceLength = 0.0f;

	/** 이 애니메이션과 연결된 스켈레톤 이름 (참조용) */
	FString SkeletonName;

	/** Skeleton signature for strict compatibility checking */
	uint64 SkeletonSignature = 0;

	/** Number of bones in target skeleton */
	int32 SkeletonBoneCount = 0;
};
