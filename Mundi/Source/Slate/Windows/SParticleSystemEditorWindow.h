#pragma once
#include "ParticleModule.h"
#include "SViewerWindow.h"

class UEditorAssetPreviewContext;
class UTexture;
class SCascadeEmittersPanel;

// Minimal Cascade-style particle editor shell
// Provides: top toolbar, left column (viewport + details), right column (emitters + curves)
// Viewport renders via ImGui::Image using the viewer state's FViewport
class SParticleSystemEditorWindow : public SViewerWindow
{
public:
    SParticleSystemEditorWindow();
    ~SParticleSystemEditorWindow() override;

    // SWindow override
    void OnRender() override;

protected:
    // SViewerWindow requirements
    ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
    void DestroyViewerState(ViewerState*& State) override;
    FString GetWindowTitle() const override { return "Cascade Particle Editor"; }

    void PreRenderViewportUpdate() override;

private:
    // Layout state
    float ColumnSplitRatio = 0.6f;      // Left vs Right
    float LeftRowSplitRatio = 0.6f;     // Viewport vs Details (left column)
    float RightRowSplitRatio = 0.45f;   // Emitters vs Curves (right column)

    void RenderLeftColumn(float width, float height);
    void RenderRightColumn(float width, float height);
    void RenderViewportArea(float width, float height);
    void RenderDetailsPanel(float width, float height);
    void RenderProperty(UParticleModule* Module, const struct FProperty* Prop);

    // Toolbar icons and helpers
    void LoadToolbarIcons();
    UTexture* IconNew = nullptr;
    UTexture* IconSave = nullptr;
    UTexture* IconLoad = nullptr;

    // Particle Editor specific icons
    UTexture* IconRestart = nullptr;
    UTexture* IconBackgroundColor = nullptr;
    UTexture* IconBounds = nullptr;
    UTexture* IconOriginAxis = nullptr;
    UTexture* IconParticle = nullptr;
    UTexture* IconLODFirst = nullptr;
    UTexture* IconLODPrev = nullptr;
    UTexture* IconLODNext = nullptr;
    UTexture* IconLODLast = nullptr;
    UTexture* IconLODInsertBefore = nullptr;
    UTexture* IconLODInsertAfter = nullptr;
    UTexture* IconLODDelete = nullptr;

    float IconSize = 24.0f;

    // Panels
    SCascadeEmittersPanel* EmittersPanel = nullptr;

    // Viewport menu state
    enum class EDetailMode { Low, Medium, High };
    EDetailMode CurrentDetailMode = EDetailMode::High;

    // Time menu state
    bool bIsPlaying = false;
    bool bRealtime = true;
    bool bLoopSimulation = true;
    float AnimSpeed = 1.0f;
};