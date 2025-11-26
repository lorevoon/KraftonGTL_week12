#pragma once
#include "Object.h"

class USkeletalMesh;
class UParticleSystem;
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
};