#pragma once
#include "FViewportClient.h"

class FParticleSystemEditorViewportClient : public FViewportClient
{
public:
    FParticleSystemEditorViewportClient();
    ~FParticleSystemEditorViewportClient() override = default;

    void Draw(FViewport* Viewport) override;
};
