#pragma once
#include "Object.h"

class USkeletalMesh;
class SWindow;
class UEditorAssetPreviewContext : public UObject
{
public:
	DECLARE_CLASS(UEditorAssetPreviewContext, UObject)

	UEditorAssetPreviewContext();

	USkeletalMesh* SkeletalMesh;
	TArray<SWindow*> ListeningWindows;
	EViewerType ViewerType = EViewerType::None;
	FString AssetPath;
};