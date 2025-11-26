#include "pch.h"
#include "SParticleSystemEditorWindow.h"
#include "SlateManager.h"
#include "Source/Runtime/Engine/Viewer/ParticleSystemEditorBootstrap.h"
#include "Source/Runtime/Renderer/FViewport.h"
#include "ResourceManager.h"
#include "SCascadeEmittersPanel.h"
#include "Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Source/Runtime/Core/Object/Property.h"
#include "Source/Runtime/Core/Misc/JsonSerializer.h"
#include "FViewportClient.h"
// Particle preview wiring
#include "Source/Runtime/Renderer/ParticleSystemEditorViewportClient.h"
#include "Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Widgets/PropertyRenderer.h"
#include "Source/Editor/PlatformProcess.h"
#include "ObjectFactory.h"
#include "Source/Runtime/Engine/Viewer/EditorAssetPreviewContext.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"

SParticleSystemEditorWindow::SParticleSystemEditorWindow()
{
    CenterRect = FRect(0, 0, 0, 0);
    EmittersPanel = new SCascadeEmittersPanel();
}

SParticleSystemEditorWindow::~SParticleSystemEditorWindow()
{
    if (EmittersPanel) { delete EmittersPanel; EmittersPanel = nullptr; }
    for (int i = 0; i < Tabs.Num(); ++i)
    {
        ViewerState* State = Tabs[i];
        ParticleSystemEditorBootstrap::DestroyViewerState(State);
    }
    Tabs.Empty();
    ActiveState = nullptr;
}

void SParticleSystemEditorWindow::OnRender()
{
    if (!ImGui::GetCurrentContext()) return;

    if (!bIsOpen)
    {
        USlateManager::GetInstance().RequestCloseDetachedWindow(this);
        return;
    }

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }
    if (bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
        bRequestFocus = false;
    }

    char UniqueTitle[256];
    FString Title = GetWindowTitle();
    sprintf_s(UniqueTitle, sizeof(UniqueTitle), "%s###%p", Title.c_str(), this);

    bool bVisible = false;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin(UniqueTitle, &bIsOpen, flags))
    {
        bVisible = true;
        bIsWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        bIsWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        // Top toolbar area specific to the particle editor
        ImGui::BeginChild("Cascade_Toolbar", ImVec2(0, 36.0f), false, ImGuiWindowFlags_NoScrollbar);
        LoadToolbarIcons();

        const ImVec2 IconSizeVec(IconSize, IconSize);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));

        // File operations - use deferred command pattern
        ImGui::SameLine();
        if (IconNew && IconNew->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_NewBtn", (void*)IconNew->GetShaderResourceView(), IconSizeVec))
            {
                PendingCommand = EParticleEditorCommand::New;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Particle System");
        }
        ImGui::SameLine();
        if (IconLoad && IconLoad->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LoadBtn", (void*)IconLoad->GetShaderResourceView(), IconSizeVec))
            {
                PendingCommand = EParticleEditorCommand::Load;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open Particle System");
        }
        ImGui::SameLine();
        if (IconSave && IconSave->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_SaveBtn", (void*)IconSave->GetShaderResourceView(), IconSizeVec))
            {
                PendingCommand = EParticleEditorCommand::Save;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save Particle System");
        }

        // Separator
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Restart simulation
        if (IconRestart && IconRestart->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_RestartBtn", (void*)IconRestart->GetShaderResourceView(), IconSizeVec))
            {
                UParticleSystemComponent* PreviewComp = GetPreviewComponent();
                if (PreviewComp)
                {
                    PreviewComp->Restart();
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restart Simulation");
        }

        // Separator
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Viewport display options
        if (IconBackgroundColor && IconBackgroundColor->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_BgColorBtn", (void*)IconBackgroundColor->GetShaderResourceView(), IconSizeVec))
            {
                ImGui::OpenPopup("BackgroundColorPicker");
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Background Color");

            // Color picker popup
            if (ImGui::BeginPopup("BackgroundColorPicker"))
            {
                ImGui::TextUnformatted("Background Color");
                ImGui::Separator();

                if (ActiveState)
                {
                    float color[4] = {
                        ActiveState->BackgroundColor.R,
                        ActiveState->BackgroundColor.G,
                        ActiveState->BackgroundColor.B,
                        ActiveState->BackgroundColor.A
                    };

                    if (ImGui::ColorPicker4("##BgColorPicker", color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
                    {
                        ActiveState->BackgroundColor.R = color[0];
                        ActiveState->BackgroundColor.G = color[1];
                        ActiveState->BackgroundColor.B = color[2];
                        ActiveState->BackgroundColor.A = color[3];
                    }
                }

                ImGui::EndPopup();
            }
        }
        ImGui::SameLine();
        if (IconBounds && IconBounds->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_BoundsBtn", (void*)IconBounds->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Bounds");
        }
        ImGui::SameLine();
        if (IconOriginAxis && IconOriginAxis->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_OriginBtn", (void*)IconOriginAxis->GetShaderResourceView(), IconSizeVec))
            {
                // Toggle axis visibility
                if (ActiveState && ActiveState->World)
                {
                    ActiveState->World->GetRenderSettings().ToggleShowFlag(EEngineShowFlags::SF_Axis);
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Origin Axis");
        }
        // ImGui::SameLine();
        // if (IconParticle && IconParticle->GetShaderResourceView())
        // {
        //     if (ImGui::ImageButton("##Cascade_ParticleBtn", (void*)IconParticle->GetShaderResourceView(), IconSizeVec)) { }
        //     if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Particles");
        // }

        // Separator
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // LOD controls
        if (IconLODFirst && IconLODFirst->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODFirstBtn", (void*)IconLODFirst->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to First LOD");
        }
        ImGui::SameLine();
        if (IconLODPrev && IconLODPrev->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODPrevBtn", (void*)IconLODPrev->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous LOD");
        }
        ImGui::SameLine();
        if (IconLODNext && IconLODNext->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODNextBtn", (void*)IconLODNext->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next LOD");
        }
        ImGui::SameLine();
        if (IconLODLast && IconLODLast->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODLastBtn", (void*)IconLODLast->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to Last LOD");
        }

        // Separator
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // LOD editing
        if (IconLODInsertBefore && IconLODInsertBefore->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODInsertBeforeBtn", (void*)IconLODInsertBefore->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Insert LOD Before");
        }
        ImGui::SameLine();
        if (IconLODInsertAfter && IconLODInsertAfter->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODInsertAfterBtn", (void*)IconLODInsertAfter->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Insert LOD After");
        }
        ImGui::SameLine();
        if (IconLODDelete && IconLODDelete->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LODDeleteBtn", (void*)IconLODDelete->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete LOD");
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        ImGui::EndChild();

        // Early out if just closed
        if (!bIsOpen)
        {
            USlateManager::GetInstance().RequestCloseDetachedWindow(this);
            ImGui::End();
            return;
        }

        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y;
        Rect.UpdateMinMax();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        const float totalW = avail.x;
        const float totalH = avail.y;
        const float splitter = 4.f;

        float leftW = totalW * ColumnSplitRatio;
        float rightW = totalW - leftW - splitter;
        if (rightW < 0) rightW = 0;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // Recompute after toolbar so layout uses remaining space
        ImVec2 availAfterToolbar = ImGui::GetContentRegionAvail();
        const float contentW = availAfterToolbar.x;
        const float contentH = availAfterToolbar.y;

        // Left Column
        ImGui::BeginChild("Cascade_LeftColumn", ImVec2(leftW, contentH), true, ImGuiWindowFlags_NoScrollbar);
        RenderLeftColumn(leftW, totalH);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // Vertical splitter
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
        ImGui::Button("##Cascade_VSplit", ImVec2(splitter, contentH));
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive())
        {
            float delta = ImGui::GetIO().MouseDelta.x;
            ColumnSplitRatio = FMath::Clamp(ColumnSplitRatio + delta / contentW, 0.35f, 0.8f);
        }

        ImGui::SameLine(0, 0);

        // Right Column
        ImGui::BeginChild("Cascade_RightColumn", ImVec2(rightW, contentH), true, ImGuiWindowFlags_NoScrollbar);
        RenderRightColumn(rightW, contentH);
        ImGui::EndChild();

        ImGui::PopStyleVar();
    }
    ImGui::End();

    // Process deferred file operations after ImGui::End()
    ProcessPendingCommands();

    // If collapsed or not visible, clear the viewport rect
    if (!bVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
        bIsWindowHovered = false;
        bIsWindowFocused = false;
    }

    if (!bIsOpen)
    {
        USlateManager::GetInstance().RequestCloseDetachedWindow(this);
    }

    bRequestFocus = false;
}

void SParticleSystemEditorWindow::RenderLeftColumn(float width, float height)
{
    const float splitter = 4.f;
    float viewportH = height * LeftRowSplitRatio;
    float detailsH = height - viewportH - splitter;
    if (detailsH < 0) detailsH = 0;

    // Viewport area (top)
    ImGui::BeginChild("Cascade_Viewport", ImVec2(width, viewportH), true, ImGuiWindowFlags_NoScrollbar);
    RenderViewportArea(width, viewportH);
    ImGui::EndChild();

    // Horizontal splitter
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
    ImGui::Button("##Cascade_HSplit_Left", ImVec2(width, splitter));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive())
    {
        float delta = ImGui::GetIO().MouseDelta.y;
        LeftRowSplitRatio = FMath::Clamp(LeftRowSplitRatio + delta / height, 0.3f, 0.8f);
    }

    // Details panel (bottom)
    ImGui::BeginChild("Cascade_Details", ImVec2(width, detailsH), true, ImGuiWindowFlags_NoScrollbar);
    RenderDetailsPanel(width, detailsH);
    ImGui::EndChild();
}

void SParticleSystemEditorWindow::RenderRightColumn(float width, float height)
{
    const float splitter = 4.f;
    float emittersH = height * RightRowSplitRatio;
    float curvesH = height - emittersH - splitter;
    if (curvesH < 0) curvesH = 0;

    // Emitters panel (top)
    ImGui::BeginChild("Cascade_Emitters", ImVec2(width, emittersH), true, ImGuiWindowFlags_NoScrollbar);
    if (EmittersPanel)
        EmittersPanel->Render(width, emittersH);
    ImGui::EndChild();

    // Horizontal splitter
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
    ImGui::Button("##Cascade_HSplit_Right", ImVec2(width, splitter));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive())
    {
        float delta = ImGui::GetIO().MouseDelta.y;
        RightRowSplitRatio = FMath::Clamp(RightRowSplitRatio + delta / height, 0.2f, 0.8f);
    }

    // Curves panel (bottom)
    ImGui::BeginChild("Cascade_Curves", ImVec2(width, curvesH), true, ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float fullW = ImGui::GetContentRegionAvail().x;
        const float padX = 10.0f;
        const float padY = 5.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 ts = ImGui::CalcTextSize("Curve Editor");
        ImVec2 chipMin = pos;
        float chipW = ts.x + padX * 2.0f; const float iconSize = 20.0f; bool hasIcon = (IconPanelCurves && IconPanelCurves->GetShaderResourceView()); if (hasIcon) chipW += iconSize + 6.0f; ImVec2 chipMax = ImVec2(pos.x + chipW, pos.y + ts.y + padY * 2.0f);
        ImU32 chipBg = ImGui::GetColorU32(ImVec4(0.18f, 0.19f, 0.23f, 1.0f));
        ImU32 chipBorder = ImGui::GetColorU32(ImVec4(0.36f, 0.40f, 0.48f, 1.0f));
        ImU32 lineCol = ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
        dl->AddRectFilled(chipMin, chipMax, chipBg, 6.0f);
        dl->AddRect(chipMin, chipMax, chipBorder, 6.0f, 0, 1.0f);
        ImVec2 cur = ImVec2(pos.x + padX, pos.y + padY); ImGui::SetCursorScreenPos(cur); if (hasIcon) { ImGui::Image((void*)IconPanelCurves->GetShaderResourceView(), ImVec2(iconSize, iconSize)); ImGui::SameLine(); cur.x += iconSize + 6.0f; } ImGui::SetCursorScreenPos(cur);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.95f, 1.00f, 1.0f));
        ImGui::TextUnformatted("Curve Editor");
        ImGui::PopStyleColor();
        float lineY = chipMax.y + 5.0f;
        dl->AddLine(ImVec2(pos.x, lineY), ImVec2(pos.x + fullW, lineY), lineCol, 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x, lineY + 6.0f));
    }
    ImGui::TextUnformatted("(Curve editor placeholder)");
    ImGui::EndChild();
}

void SParticleSystemEditorWindow::RenderViewportArea(float width, float height)
{
    // Position for the viewport texture
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Compute actual available space inside child
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float actualW = avail.x;
    float actualH = avail.y;

    // Update viewport rect for input mapping
    CenterRect.Left = pos.x;
    CenterRect.Top = pos.y;
    CenterRect.Right = pos.x + actualW;
    CenterRect.Bottom = pos.y + actualH;
    CenterRect.UpdateMinMax();

    // Render to texture and display via ImGui::Image
    OnRenderViewport();

    if (ActiveState && ActiveState->Viewport)
    {
        if (ID3D11ShaderResourceView* SRV = ActiveState->Viewport->GetSRV())
        {
            ImGui::Image((void*)SRV, ImVec2(actualW, actualH));
            ActiveState->Viewport->SetViewportHovered(ImGui::IsItemHovered());
        }
        else
        {
            ImGui::Dummy(ImVec2(actualW, actualH));
            ActiveState->Viewport->SetViewportHovered(false);
        }
    }
    else
    {
        ImGui::Dummy(ImVec2(actualW, actualH));
    }

    // Overlay menu buttons at top left (like Unreal Engine Cascade)
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 4, pos.y + 4));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.7f)); // Translucent dark background
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text

    // View menu button
    if (ImGui::Button("View"))
    {
        ImGui::OpenPopup("ViewMenu");
    }

    if (ImGui::BeginPopup("ViewMenu"))
    {
        // Get current view mode from viewport client
        EViewMode CurrentViewMode = EViewMode::VMI_Lit_Phong;
        if (ActiveState && ActiveState->Client)
        {
            CurrentViewMode = ActiveState->Client->GetViewMode();
        }

        // View Modes submenu
        if (ImGui::BeginMenu("View Modes"))
        {
            if (ImGui::MenuItem("Wireframe", nullptr, CurrentViewMode == EViewMode::VMI_Wireframe))
            {
                if (ActiveState && ActiveState->Client)
                {
                    ActiveState->Client->SetViewMode(EViewMode::VMI_Wireframe);
                }
            }
            if (ImGui::MenuItem("Unlit", nullptr, CurrentViewMode == EViewMode::VMI_Unlit))
            {
                if (ActiveState && ActiveState->Client)
                {
                    ActiveState->Client->SetViewMode(EViewMode::VMI_Unlit);
                }
            }
            if (ImGui::MenuItem("Lit (Phong)", nullptr, CurrentViewMode == EViewMode::VMI_Lit_Phong))
            {
                if (ActiveState && ActiveState->Client)
                {
                    ActiveState->Client->SetViewMode(EViewMode::VMI_Lit_Phong);
                }
            }
            if (ImGui::MenuItem("Lit (Gouraud)", nullptr, CurrentViewMode == EViewMode::VMI_Lit_Gouraud))
            {
                if (ActiveState && ActiveState->Client)
                {
                    ActiveState->Client->SetViewMode(EViewMode::VMI_Lit_Gouraud);
                }
            }
            if (ImGui::MenuItem("Lit (Lambert)", nullptr, CurrentViewMode == EViewMode::VMI_Lit_Lambert))
            {
                if (ActiveState && ActiveState->Client)
                {
                    ActiveState->Client->SetViewMode(EViewMode::VMI_Lit_Lambert);
                }
            }
            ImGui::EndMenu();
        }

        // Detail Modes submenu
        if (ImGui::BeginMenu("Detail Modes"))
        {
            if (ImGui::MenuItem("Low", nullptr, CurrentDetailMode == EDetailMode::Low))
            {
                CurrentDetailMode = EDetailMode::Low;
            }
            if (ImGui::MenuItem("Medium", nullptr, CurrentDetailMode == EDetailMode::Medium))
            {
                CurrentDetailMode = EDetailMode::Medium;
            }
            if (ImGui::MenuItem("High", nullptr, CurrentDetailMode == EDetailMode::High))
            {
                CurrentDetailMode = EDetailMode::High;
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // Get current grid flag from render settings
        bool bShowGrid = false;
        if (ActiveState && ActiveState->World)
        {
            bShowGrid = ActiveState->World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Grid);
        }

        // Grid toggle
        if (ImGui::MenuItem("Grid", nullptr, bShowGrid))
        {
            if (ActiveState && ActiveState->World)
            {
                ActiveState->World->GetRenderSettings().ToggleShowFlag(EEngineShowFlags::SF_Grid);
            }
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Time menu button
    if (ImGui::Button("Time"))
    {
        ImGui::OpenPopup("TimeMenu");
    }

    if (ImGui::BeginPopup("TimeMenu"))
    {
        // Play/Pause toggle
        if (bIsPlaying)
        {
            if (ImGui::MenuItem("Pause"))
            {
                bIsPlaying = false;
                ApplyTimeSettings();
            }
        }
        else
        {
            if (ImGui::MenuItem("Play"))
            {
                bIsPlaying = true;
                ApplyTimeSettings();
            }
        }
        
        ImGui::Separator();

        // AnimSpeed submenu
        if (ImGui::BeginMenu("AnimSpeed"))
        {
            if (ImGui::MenuItem("100%", nullptr, AnimSpeed == 1.0f))
            {
                AnimSpeed = 1.0f;
                ApplyTimeSettings();
            }
            if (ImGui::MenuItem("50%", nullptr, AnimSpeed == 0.5f))
            {
                AnimSpeed = 0.5f;
                ApplyTimeSettings();
            }
            if (ImGui::MenuItem("25%", nullptr, AnimSpeed == 0.25f))
            {
                AnimSpeed = 0.25f;
                ApplyTimeSettings();
            }
            if (ImGui::MenuItem("10%", nullptr, AnimSpeed == 0.1f))
            {
                AnimSpeed = 0.1f;
                ApplyTimeSettings();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("200%", nullptr, AnimSpeed == 2.0f))
            {
                AnimSpeed = 2.0f;
                ApplyTimeSettings();
            }
            if (ImGui::MenuItem("500%", nullptr, AnimSpeed == 5.0f))
            {
                AnimSpeed = 5.0f;
                ApplyTimeSettings();
            }
            if (ImGui::MenuItem("1000%", nullptr, AnimSpeed == 10.0f))
            {
                AnimSpeed = 10.0f;
                ApplyTimeSettings();
            }

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

ViewerState* SParticleSystemEditorWindow::CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context)
{
    ViewerState* NewState = ParticleSystemEditorBootstrap::CreateViewerState(Name, World, Device);
    if (!NewState) return nullptr;

    // Context에서 ParticleSystem을 가져와서 편집
    if (Context)
    {
        UParticleSystem* PS = nullptr;

        // 포인터가 직접 전달된 경우 우선 사용 (하드코딩된 ParticleSystem 지원)
        if (Context->ParticleSystem)
        {
            PS = Context->ParticleSystem;
        }
        // AssetPath가 있으면 로드 (preload된 리소스는 캐시에서 반환)
        else if (!Context->AssetPath.empty())
        {
            PS = UResourceManager::GetInstance().Load<UParticleSystem>(Context->AssetPath);
        }

        if (PS && EmittersPanel)
        {
            EmittersPanel->SetEditingSystem(PS);
            NewState->LoadedMeshPath = Context->AssetPath;  // 탭 매칭용
        }

        // Scene에서 열린 경우 소스 컴포넌트 추적
        // 다른 이름으로 저장 시 이 컴포넌트의 Template도 업데이트해야 함
        SourceSceneComponent = Cast<UParticleSystemComponent>(Context->SourceComponent);
    }
    else
    {
        SourceSceneComponent = nullptr;
    }
    return NewState;
}

void SParticleSystemEditorWindow::DestroyViewerState(ViewerState*& State)
{
    ParticleSystemEditorBootstrap::DestroyViewerState(State);
}

void SParticleSystemEditorWindow::PreRenderViewportUpdate()
{
    // Ensure the viewport's preview component uses the EditingSystem template
    if (!ActiveState || !EmittersPanel)
        return;

    FViewportClient* BaseClient = ActiveState->Client;
    FParticleSystemEditorViewportClient* PSClient = dynamic_cast<FParticleSystemEditorViewportClient*>(BaseClient);
    if (!PSClient)
        return;

    UParticleSystemComponent* PreviewComp = PSClient->GetPreviewComponent();
    if (!PreviewComp && ActiveState->World)
    {
        // Try to find any particle system component in the preview world
        PreviewComp = ActiveState->World->FindComponent<UParticleSystemComponent>();
        if (PreviewComp)
        {
            PSClient->SetPreviewComponent(PreviewComp);
        }
    }

    if (!PreviewComp)
        return;

    // If the emitters panel has no system, adopt the preview component's template
    if (EmittersPanel)
    {
        UParticleSystem* CurrentEditing = EmittersPanel->GetEditingSystem();
        UParticleSystem* PreviewTemplate = PreviewComp->Template;
        if (!CurrentEditing && PreviewTemplate)
        {
            EmittersPanel->SetEditingSystem(PreviewTemplate);
        }
    }

    UParticleSystem* EditingSystem = EmittersPanel->GetEditingSystem();
    // // Only override the preview when the editing system is actually populated
    // if (EditingSystem && PreviewComp->Template != EditingSystem)
    if (EditingSystem)
    {
        if (PreviewComp->Template != EditingSystem)
        {
            PreviewComp->SetTemplate(EditingSystem);
            PreviewComp->Restart();
            // Clear dirty state once bound
            EditingSystem->bIsDirty = false;
            // Apply time settings to newly bound component
            ApplyTimeSettings();
        }
        else if (EditingSystem->bIsDirty)
        {
            // Rebuild instances to reflect structural/property changes
            PreviewComp->RebuildInstances();
            PreviewComp->Restart();
            EditingSystem->bIsDirty = false;
        }
    }

    // Sync editor visibility/render mode state to emitter instances
    SyncEmitterEditorStates(PreviewComp);
}

void SParticleSystemEditorWindow::LoadToolbarIcons()
{
    // File operations
    if (!IconNew)
        IconNew = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_New.png");
    if (!IconSave)
        IconSave = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_Save.png");
    if (!IconLoad)
        IconLoad = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_Load.png");

    // Particle Editor specific icons
    if (!IconRestart)
        IconRestart = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_Restart.png");
    if (!IconBackgroundColor)
        IconBackgroundColor = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_BackgroundColor.png");
    if (!IconBounds)
        IconBounds = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_Bounds.png");
    if (!IconOriginAxis)
        IconOriginAxis = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_OriginAxis.png");
    if (!IconParticle)
        IconParticle = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_Particle.png");
    if (!IconLODFirst)
        IconLODFirst = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODFirst.png");
    if (!IconLODPrev)
        IconLODPrev = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODPrev.png");
    if (!IconLODNext)
        IconLODNext = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODNext.png");
    if (!IconLODLast)
        IconLODLast = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODLast.png");
    if (!IconLODInsertBefore)
        IconLODInsertBefore = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODInsertBefore.png");
    if (!IconLODInsertAfter)
        IconLODInsertAfter = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODInsertAfter.png");
    if (!IconLODDelete)
        IconLODDelete = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Toolbar_LODDelete.png");

    // Panel icons
    if (!IconPanelDetails)
        IconPanelDetails = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Panel_Details.png");
    if (!IconPanelCurves)
        IconPanelCurves = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Particle Editor/Panel_CurveEditor.png");
}

void SParticleSystemEditorWindow::RenderDetailsPanel(float width, float height)
{
    // Chip-style header for Details
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float fullW = ImGui::GetContentRegionAvail().x;
        const float padX = 10.0f;
        const float padY = 5.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 ts = ImGui::CalcTextSize("Details");
        ImVec2 chipMin = pos;
        float chipW = ts.x + padX * 2.0f; const float iconSize = 20.0f; bool hasIcon = (IconPanelDetails && IconPanelDetails->GetShaderResourceView()); if (hasIcon) chipW += iconSize + 6.0f; ImVec2 chipMax = ImVec2(pos.x + chipW, pos.y + ts.y + padY * 2.0f);
        ImU32 chipBg = ImGui::GetColorU32(ImVec4(0.18f, 0.19f, 0.23f, 1.0f));
        ImU32 chipBorder = ImGui::GetColorU32(ImVec4(0.36f, 0.40f, 0.48f, 1.0f));
        ImU32 lineCol = ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
        dl->AddRectFilled(chipMin, chipMax, chipBg, 6.0f);
        dl->AddRect(chipMin, chipMax, chipBorder, 6.0f, 0, 1.0f);
        ImVec2 cur = ImVec2(pos.x + padX, pos.y + padY); ImGui::SetCursorScreenPos(cur); if (hasIcon) { ImGui::Image((void*)IconPanelDetails->GetShaderResourceView(), ImVec2(iconSize, iconSize)); ImGui::SameLine(); cur.x += iconSize + 6.0f; } ImGui::SetCursorScreenPos(cur);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 1.00f, 1.0f));
        ImGui::TextUnformatted("Details");
        ImGui::PopStyleColor();
        float lineY = chipMax.y + 5.0f;
        dl->AddLine(ImVec2(pos.x, lineY), ImVec2(pos.x + fullW, lineY), lineCol, 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x, lineY + 6.0f));
    }

    if (!EmittersPanel)
    {
        ImGui::TextUnformatted("(No emitters panel)");
        return;
    }

    UParticleModule* SelectedModule = EmittersPanel->GetSelectedModule();
    int32 SelectedEmitterIndex = EmittersPanel->GetSelectedEmitterIndex();
    UParticleSystem* EditingSystem = EmittersPanel->GetEditingSystem();

    // If no module is selected, show emitter or system properties
    if (!SelectedModule)
    {
        // If an emitter is selected, show its properties
        if (SelectedEmitterIndex >= 0 && EditingSystem)
        {
            UParticleEmitter* SelectedEmitter = EditingSystem->GetEmitter(SelectedEmitterIndex);
            if (SelectedEmitter)
            {
                UClass* EmitterClass = SelectedEmitter->GetClass();
                if (!EmitterClass)
                {
                    ImGui::TextUnformatted("(Invalid emitter class)");
                    return;
                }

                // Display emitter name header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.5f, 1.0f)); // Orange for emitter
                ImGui::Text("Emitter: %s", SelectedEmitter->EmitterName.c_str());
                ImGui::PopStyleColor();
                ImGui::Separator();

                // Get all properties
                const TArray<FProperty>& Properties = EmitterClass->GetAllProperties();

                // Group properties by category
                TMap<FString, TArray<const FProperty*>> CategorizedProperties;
                for (const FProperty& Prop : Properties)
                {
                    if (Prop.bIsEditAnywhere)
                    {
                        FString Category = Prop.Category ? FString(Prop.Category) : FString("General");
                        if (!CategorizedProperties.Contains(Category))
                        {
                            CategorizedProperties.Add(Category, TArray<const FProperty*>());
                        }
                        CategorizedProperties[Category].Add(&Prop);
                    }
                }

                // Render properties by category
                for (auto& Pair : CategorizedProperties)
                {
                    const FString& CategoryName = Pair.first;
                    const TArray<const FProperty*>& CategoryProps = Pair.second;

                    // Category header
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
                    ImGui::TextUnformatted(CategoryName.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Separator();

                    // Render each property
                    for (const FProperty* Prop : CategoryProps)
                    {
                        RenderEmitterProperty(SelectedEmitter, Prop);
                    }

                    ImGui::Dummy(ImVec2(0, 8)); // Spacing between categories
                }
                return;
            }
        }

        // No emitter selected either, show UParticleSystem properties
        if (!EditingSystem)
        {
            ImGui::TextUnformatted("Select a module to view properties");
            return;
        }

        // Show UParticleSystem properties
        UClass* SystemClass = EditingSystem->GetClass();
        if (!SystemClass)
        {
            ImGui::TextUnformatted("(Invalid system class)");
            return;
        }

        // Display system name header
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.9f, 1.0f)); // Cyan for system
        ImGui::TextUnformatted("Particle System");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Get all properties
        const TArray<FProperty>& Properties = SystemClass->GetAllProperties();

        // Group properties by category
        TMap<FString, TArray<const FProperty*>> CategorizedProperties;
        for (const FProperty& Prop : Properties)
        {
            if (Prop.bIsEditAnywhere)
            {
                FString Category = Prop.Category ? FString(Prop.Category) : FString("General");
                if (!CategorizedProperties.Contains(Category))
                {
                    CategorizedProperties.Add(Category, TArray<const FProperty*>());
                }
                CategorizedProperties[Category].Add(&Prop);
            }
        }

        // Render properties by category
        for (auto& Pair : CategorizedProperties)
        {
            const FString& CategoryName = Pair.first;
            const TArray<const FProperty*>& CategoryProps = Pair.second;

            // Category header
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
            ImGui::TextUnformatted(CategoryName.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();

            // Render each property
            for (const FProperty* Prop : CategoryProps)
            {
                RenderSystemProperty(EditingSystem, Prop);
            }

            ImGui::Dummy(ImVec2(0, 8)); // Spacing between categories
        }
        return;
    }

    // Get the module's class and properties
    UClass* ModuleClass = SelectedModule->GetClass();
    if (!ModuleClass)
    {
        ImGui::TextUnformatted("(Invalid module class)");
        return;
    }

    // Display module name header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.5f, 1.0f));
    ImGui::TextUnformatted(SelectedModule->ModuleName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Get all properties including inherited ones
    const TArray<FProperty>& Properties = ModuleClass->GetAllProperties();

    // Group properties by category
    TMap<FString, TArray<const FProperty*>> CategorizedProperties;
    for (const FProperty& Prop : Properties)
    {
        if (Prop.bIsEditAnywhere)
        {
            FString Category = Prop.Category ? FString(Prop.Category) : FString("General");
            if (!CategorizedProperties.Contains(Category))
            {
                CategorizedProperties.Add(Category, TArray<const FProperty*>());
            }
            CategorizedProperties[Category].Add(&Prop);
        }
    }

    // Render properties by category
    for (auto& Pair : CategorizedProperties)
    {
        const FString& CategoryName = Pair.first;
        const TArray<const FProperty*>& CategoryProps = Pair.second;

        // Category header
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
        ImGui::TextUnformatted(CategoryName.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Render each property
        for (const FProperty* Prop : CategoryProps)
        {
            RenderProperty(SelectedModule, Prop);
        }

        ImGui::Dummy(ImVec2(0, 8)); // Spacing between categories
    }
}

void SParticleSystemEditorWindow::RenderProperty(UParticleModule* Module, const FProperty* Prop)
{
    if (!Module || !Prop)
        return;

    ImGui::PushID(Prop->Name);
    bool bChanged = UPropertyRenderer::RenderProperty(*Prop, Module);
    if (bChanged && EmittersPanel && EmittersPanel->GetEditingSystem())
    {
        EmittersPanel->GetEditingSystem()->bIsDirty = true;
    }
    ImGui::PopID();
}

void SParticleSystemEditorWindow::RenderSystemProperty(UParticleSystem* System, const FProperty* Prop)
{
    if (!System || !Prop)
        return;

    ImGui::PushID(Prop->Name);
    bool bChanged = UPropertyRenderer::RenderProperty(*Prop, System);
    if (bChanged && System)
    {
        System->bIsDirty = true;
    }
    ImGui::PopID();
}

void SParticleSystemEditorWindow::RenderEmitterProperty(UParticleEmitter* Emitter, const FProperty* Prop)
{
    if (!Emitter || !Prop)
        return;

    ImGui::PushID(Prop->Name);
    bool bChanged = UPropertyRenderer::RenderProperty(*Prop, Emitter);
    if (bChanged && EmittersPanel && EmittersPanel->GetEditingSystem())
    {
        EmittersPanel->GetEditingSystem()->bIsDirty = true;
    }
    ImGui::PopID();
}

UParticleSystemComponent* SParticleSystemEditorWindow::GetPreviewComponent() const
{
    if (!ActiveState || !ActiveState->Client)
        return nullptr;

    FParticleSystemEditorViewportClient* PSClient = dynamic_cast<FParticleSystemEditorViewportClient*>(ActiveState->Client);
    if (!PSClient)
        return nullptr;

    return PSClient->GetPreviewComponent();
}

void SParticleSystemEditorWindow::ApplyTimeSettings()
{
    UParticleSystemComponent* PreviewComp = GetPreviewComponent();
    if (!PreviewComp)
        return;

    // Calculate effective playback rate
    // If paused, set to 0. If playing, use AnimSpeed.
    // If not realtime, we could potentially step manually, but for now just pause.
    float EffectiveRate = 0.0f;
    if (bIsPlaying && bRealtime)
    {
        EffectiveRate = AnimSpeed;
    }

    PreviewComp->CustomPlaybackRate = EffectiveRate;
}

void SParticleSystemEditorWindow::SyncEmitterEditorStates(UParticleSystemComponent* PreviewComp)
{
    if (!PreviewComp || !EmittersPanel)
        return;

    // Check if any emitter has solo enabled
    bool bAnySolo = EmittersPanel->HasAnySoloEmitter();

    // Sync visibility and render mode from editor states to emitter instances
    int32 NumInstances = PreviewComp->EmitterInstances.Num();
    for (int32 i = 0; i < NumInstances; ++i)
    {
        FParticleEmitterInstance* Instance = PreviewComp->EmitterInstances[i];
        if (!Instance)
            continue;

        FEmitterEditorState& EditorState = EmittersPanel->GetEmitterEditorState(i);

        // Determine visibility based on solo mode
        if (bAnySolo)
        {
            // If any emitter is solo'd, only show solo'd emitters that are also visible
            Instance->bEditorVisible = EditorState.bIsSolo && EditorState.bIsVisible;
        }
        else
        {
            // Normal visibility check
            Instance->bEditorVisible = EditorState.bIsVisible;
        }

        // Sync render mode
        Instance->EditorRenderMode = static_cast<EEmitterRenderMode>(EditorState.RenderMode);
    }
}

void SParticleSystemEditorWindow::ProcessPendingCommands()
{
    if (PendingCommand == EParticleEditorCommand::None)
        return;

    EParticleEditorCommand Command = PendingCommand;
    PendingCommand = EParticleEditorCommand::None;

    switch (Command)
    {
    case EParticleEditorCommand::New:
        OnNewParticleSystem();
        break;
    case EParticleEditorCommand::Load:
        OnLoadParticleSystem();
        break;
    case EParticleEditorCommand::Save:
        OnSaveParticleSystem();
        break;
    default:
        break;
    }
}

void SParticleSystemEditorWindow::OnNewParticleSystem()
{
    // Create a new particle system
    UParticleSystem* NewSystem = ObjectFactory::NewObject<UParticleSystem>();
    if (NewSystem && EmittersPanel)
    {
        EmittersPanel->SetEditingSystem(NewSystem);
        NewSystem->bIsDirty = true;
    }
}

void SParticleSystemEditorWindow::OnLoadParticleSystem()
{
    std::filesystem::path FilePath = FPlatformProcess::OpenLoadFileDialog(
        L"Data/ParticleSystems",
        L".json",
        L"Particle System (*.json)"
    );

    if (FilePath.empty())
        return;

    FString FilePathStr = FilePath.string();

    // Load JSON from file
    std::ifstream File(FilePath);
    if (!File.is_open())
    {
        UE_LOG("[ParticleEditor] Failed to open file: %s", FilePathStr.c_str());
        return;
    }

    std::stringstream Buffer;
    Buffer << File.rdbuf();
    File.close();

    JSON LoadedJson = JSON::Load(Buffer.str());
    if (LoadedJson.IsNull())
    {
        UE_LOG("[ParticleEditor] Failed to parse JSON from file: %s", FilePathStr.c_str());
        return;
    }

    // Create new particle system and deserialize
    UParticleSystem* LoadedSystem = ObjectFactory::NewObject<UParticleSystem>();
    if (!LoadedSystem)
    {
        UE_LOG("[ParticleEditor] Failed to create UParticleSystem");
        return;
    }

    LoadedSystem->Serialize(true, LoadedJson);

    // Set file path for future saves
    LoadedSystem->SetFilePath(FilePathStr);

    if (EmittersPanel)
    {
        EmittersPanel->SetEditingSystem(LoadedSystem);
    }

    UE_LOG("[ParticleEditor] Loaded particle system from: %s", FilePathStr.c_str());
}

void SParticleSystemEditorWindow::OnSaveParticleSystem()
{
    if (!EmittersPanel)
        return;

    UParticleSystem* EditingSystem = EmittersPanel->GetEditingSystem();
    if (!EditingSystem)
    {
        UE_LOG("[ParticleEditor] No particle system to save");
        return;
    }

    // Get default filename from existing path or generate one
    FWideString DefaultFileName = L"NewParticleSystem";
    FString ExistingPath = EditingSystem->GetFilePath();
    if (!ExistingPath.empty())
    {
        std::filesystem::path ExistingFsPath(ExistingPath.c_str());
        DefaultFileName = ExistingFsPath.stem().wstring();
    }

    std::filesystem::path FilePath = FPlatformProcess::OpenSaveFileDialog(
        L"Data/ParticleSystems",
        L".json",
        L"Particle System (*.json)",
        DefaultFileName
    );

    if (FilePath.empty())
        return;

    // Ensure .json extension
    if (FilePath.extension() != ".json")
    {
        FilePath += ".json";
    }

    FString FilePathStr = FilePath.string();

    // Serialize to JSON
    JSON SaveJson = JSON::Make(JSON::Class::Object);
    EditingSystem->Serialize(false, SaveJson);

    // Write to file
    std::ofstream File(FilePath);
    if (!File.is_open())
    {
        UE_LOG("[ParticleEditor] Failed to create file: %s", FilePathStr.c_str());
        return;
    }

    File << SaveJson.dump(2);
    File.close();

    // Update the system's file path (��� ��η� ����ȭ)
    FString NormalizedPath = NormalizePath(FilePathStr);

    // ���� ��ο� ���Ͽ� �ٸ� �̸�/��η� ������ ��� �� ������ �ε��Ͽ� ResourceManager�� ���
    FString NormalizedExistingPath = NormalizePath(ExistingPath);

    if (NormalizedPath != NormalizedExistingPath)
    {
        // ���� ������ ������ ResourceManager�� ���� �ε� (�� ��ü�� ��ϵ�)
        UParticleSystem* NewSystem = UResourceManager::GetInstance().Load<UParticleSystem>(NormalizedPath);

        // ĳ�� ���� (���� ������ �� ���� �����)
        UPropertyRenderer::ClearResourcesCache();

        // ���� ������ ParticleSystem�� ���� Template���� ����
        UParticleSystemComponent* PreviewComp = GetPreviewComponent();

        if (PreviewComp && NewSystem)
        {
            PreviewComp->SetTemplate(NewSystem);

            // EmittersPanel�� EditingSystem�� ������Ʈ (PreRenderViewportUpdate���� ����� ����)
            if (EmittersPanel)
            {
                EmittersPanel->SetEditingSystem(NewSystem);
            }

            // Scene�� �ҽ� ������Ʈ ������Ʈ
            // Property Window���� "Cascade Editor" ��ư���� ���� ��� �ش� ������Ʈ�� Template�� ������Ʈ
            if (SourceSceneComponent)
            {
                SourceSceneComponent->SetTemplate(NewSystem);
            }
        }
    }
    else
    {
        // ���� ��η� ������� ��쿡�� FilePath ������Ʈ
        EditingSystem->SetFilePath(NormalizedPath);
    }

    UE_LOG("[ParticleEditor] Saved particle system to: %s", NormalizedPath.c_str());
}
