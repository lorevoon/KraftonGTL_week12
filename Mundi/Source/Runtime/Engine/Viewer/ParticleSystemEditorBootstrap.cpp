#include "pch.h"
#include "ParticleSystemEditorBootstrap.h"
#include "FViewport.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"
#include "Source/Runtime/Renderer/ParticleSystemEditorViewportClient.h"

ViewerState* ParticleSystemEditorBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ViewerState* State = new ViewerState();
    State->Name = Name ? Name : "Cascade";

    // Preview world
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);
    State->World->Initialize();
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    // Viewport
    State->Viewport = new FViewport();
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);
    State->Viewport->SetUseRenderTarget(true); // Render to texture for ImGui::Image

    // Client
    auto* Client = new FParticleSystemEditorViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);
    Client->SetViewerState(State);
    State->Client = Client;
    State->Viewport->SetViewportClient(Client);
    State->World->SetEditorCameraActor(Client->GetCamera());

    // No default preview actor yet (particle system coming later)

    return State;
}

void ParticleSystemEditorBootstrap::DestroyViewerState(ViewerState*& State)
{
    if (!State) return;
    if (State->Viewport) { delete State->Viewport; State->Viewport = nullptr; }
    if (State->Client) { delete State->Client; State->Client = nullptr; }
    if (State->World) { ObjectFactory::DeleteObject(State->World); State->World = nullptr; }
    delete State; State = nullptr;
}

