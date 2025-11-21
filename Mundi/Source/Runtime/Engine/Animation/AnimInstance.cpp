#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"

UAnimInstance::UAnimInstance() = default;

void UAnimInstance::InitializeAnimation(USkeletalMeshComponent* InComponent)
{
    OwningComponent = InComponent;
    bInitialized = (OwningComponent != nullptr);
    if (bInitialized)
    {
        NativeInitializeAnimation();
    }
}

void UAnimInstance::NativeInitializeAnimation()
{
    // Base implementation: no-op
}

void UAnimInstance::Tick(float DeltaTime)
{
    if (!OwningComponent)
    {
        return;
    }

    const USkeletalMesh* Mesh = OwningComponent->GetSkeletalMesh();
    const FSkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
    if (!Skeleton)
    {
        return;
    }

    // Update phase
    NativeUpdateAnimation(DeltaTime);

    // Evaluate phase (build a local-space pose)
    FPoseContext Output;
    Output.Initialize(OwningComponent, Skeleton, DeltaTime);
    EvaluateAnimation(Output);

    // Note: Applying Output.LocalSpacePose to the component is handled by the component integration step
    // (USkeletalMeshComponent::Tick or a dedicated ApplyPose path). Keeping this decoupled avoids
    // accessing protected members like ForceRecomputePose() here.
}

void UAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    // Base implementation: no-op
}

void UAnimInstance::EvaluateAnimation(FPoseContext& Output)
{
    // Base behavior: output reference/bind pose
    Output.ResetToRefPose();
}

USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const
{
    return OwningComponent;
}

const FSkeleton* UAnimInstance::GetSkeleton() const
{
    const USkeletalMesh* Mesh = OwningComponent ? OwningComponent->GetSkeletalMesh() : nullptr;
    return Mesh ? Mesh->GetSkeleton() : nullptr;
}

bool UAnimInstance::IsPlaying() const
{
    return false;
}


