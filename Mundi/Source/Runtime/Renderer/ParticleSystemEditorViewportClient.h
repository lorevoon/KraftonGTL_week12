#pragma once
#include "FViewportClient.h"
class UParticleSystemComponent;

class FParticleSystemEditorViewportClient : public FViewportClient
{
public:
    FParticleSystemEditorViewportClient();
    ~FParticleSystemEditorViewportClient() override = default;

    void Draw(FViewport* Viewport) override;

    // Preview particle system component used in the editor viewport
    void SetPreviewComponent(UParticleSystemComponent* InComponent) { PreviewComponent = InComponent; }
    UParticleSystemComponent* GetPreviewComponent() const { return PreviewComponent; }

private:
    UParticleSystemComponent* PreviewComponent = nullptr;
};
