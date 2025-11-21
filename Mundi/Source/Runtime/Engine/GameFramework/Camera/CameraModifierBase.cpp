#include "pch.h"
#include "CameraModifierBase.h"

IMPLEMENT_CLASS(UCameraModifierBase);

UCameraModifierBase::UCameraModifierBase() = default;

void UCameraModifierBase::ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo)
{
}

void UCameraModifierBase::CollectPostProcess(TArray<FPostProcessModifier>& OutModifiers)
{
}

