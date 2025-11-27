#include "pch.h"
#include "ParticleSystemEditorViewportClient.h"
#include "CameraActor.h"

FParticleSystemEditorViewportClient::FParticleSystemEditorViewportClient()
{
    ViewportType = EViewportType::Perspective;
    ViewMode = EViewMode::VMI_Lit_Phong;

    Camera->SetActorLocation(FVector(5.0f, 0.0f, 2.0f)); // Front view, slightly elevated
    Camera->SetActorRotation(FVector(0.0f, 0.0f, 180.0f)); // Front view, slightly elevated

    Camera->SetCameraPitch(0.0f);   // Pitch: 0 degrees (level)
    Camera->SetCameraYaw(180.0f);   // Yaw: 180 degrees (facing +X direction from -X position)
}

void FParticleSystemEditorViewportClient::Draw(FViewport* Viewport)
{
    FViewportClient::Draw(Viewport);
}
