#include "pch.h"
#include "ParticleSystemEditorViewportClient.h"
#include "CameraActor.h"

FParticleSystemEditorViewportClient::FParticleSystemEditorViewportClient()
{
    ViewportType = EViewportType::Perspective;
    ViewMode = EViewMode::VMI_Lit_Phong;

    Camera->SetActorLocation(FVector(250.0f, -200.0f, 150.0f));
    Camera->SetActorRotation(FVector(-15.0f, 0.0f, 45.0f));
    Camera->SetCameraPitch(-15.0f);
    Camera->SetCameraYaw(45.0f);
}

void FParticleSystemEditorViewportClient::Draw(FViewport* Viewport)
{
    FViewportClient::Draw(Viewport);
}
