#pragma once
#include "AnimNodeBase.h"
#include "SkeletalMeshComponent.h"
#include "UAnimInstance.generated.h"

UCLASS(DisplayName="애니메이션 인스턴스", Description="")
class UAnimInstance : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UAnimInstance();

    void InitializeAnimation(USkeletalMeshComponent* InComponent);
    virtual void NativeInitializeAnimation();

    void Tick(float DeltaTime);
    virtual void NativeUpdateAnimation(float DeltaTime);
    virtual void EvaluateAnimation(FPoseContext& Output);
    virtual bool IsPlaying() const; // default: false

    USkeletalMeshComponent* GetOwningComponent() const;
    const FSkeleton* GetSkeleton() const;
    
private:
    USkeletalMeshComponent* OwningComponent = nullptr;
    bool bInitialized = false;
};
