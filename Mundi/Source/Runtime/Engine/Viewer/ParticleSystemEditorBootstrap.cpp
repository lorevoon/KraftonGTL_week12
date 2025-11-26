#include "pch.h"
#include "ParticleSystemEditorBootstrap.h"
#include "FViewport.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"
#include "Source/Runtime/Renderer/ParticleSystemEditorViewportClient.h"
// Preview actor/component
#include "ParticleActor.h"
#include "Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

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
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_Grid);

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

    // Spawn a preview particle actor in the preview world and cache component on the client
    if (State->World)
    {
        AParticleActor* PreviewPSActor = State->World->SpawnActor<AParticleActor>();
        if (PreviewPSActor)
        {
            if (UParticleSystemComponent* PSC = PreviewPSActor->GetParticleSystemComponent())
            {
                PSC->bAutoActivate = true;
                PSC->Activate();
                Client->SetPreviewComponent(PSC);
            }
        }
    }

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

