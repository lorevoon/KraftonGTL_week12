#pragma once
#include "Object.h"

class USkeletalMesh;
class UParticleSystem;
class UActorComponent;
class SWindow;
class UEditorAssetPreviewContext : public UObject
{
public:
	DECLARE_CLASS(UEditorAssetPreviewContext, UObject)

	UEditorAssetPreviewContext();

	USkeletalMesh* SkeletalMesh = nullptr;
	UParticleSystem* ParticleSystem = nullptr;
	TArray<SWindow*> ListeningWindows;
	EViewerType ViewerType = EViewerType::None;
	FString AssetPath;

	// Source component that opened the viewer (for save-back capability)
	// 다른 이름으로 저장 시 이 컴포넌트의 Template도 업데이트됨
	UActorComponent* SourceComponent = nullptr;
};