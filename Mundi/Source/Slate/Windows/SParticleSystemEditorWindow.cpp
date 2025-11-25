#include "pch.h"
#include "SParticleSystemEditorWindow.h"
#include "SlateManager.h"
#include "Source/Runtime/Engine/Viewer/ParticleSystemEditorBootstrap.h"
#include "Source/Runtime/Renderer/FViewport.h"
#include "ResourceManager.h"
#include "SCascadeEmittersPanel.h"
#include "Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Source/Runtime/Core/Object/Property.h"
#include "Source/Runtime/Core/Misc/JsonSerializer.h"
#include "FViewportClient.h"
// Particle preview wiring
#include "Source/Runtime/Renderer/ParticleSystemEditorViewportClient.h"
#include "Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

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

        // File operations
        ImGui::SameLine();
        if (IconNew && IconNew->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_NewBtn", (void*)IconNew->GetShaderResourceView(), IconSizeVec))
            {
                // Create new particle system
                if (EmittersPanel)
                {
                    UParticleSystem* NewSystem = NewObject<UParticleSystem>();
                    NewSystem->SystemName = "NewParticleSystem";
                    EmittersPanel->SetEditingSystem(NewSystem);
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Particle System");
        }
        ImGui::SameLine();
        if (IconLoad && IconLoad->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LoadBtn", (void*)IconLoad->GetShaderResourceView(), IconSizeVec))
            {
                // Open file dialog
                OPENFILENAMEA ofn = {};
                char szFile[260] = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Particle System Files\0*.particle\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = "Data\\ParticleSystems";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileNameA(&ofn) == TRUE)
                {
                    // Load via ResourceManager as a resource for caching/path identity
                    std::string narrowPath(szFile);
                    UParticleSystem* LoadedSystem = UResourceManager::GetInstance().Load<UParticleSystem>(narrowPath);
                    if (LoadedSystem && EmittersPanel)
                    {
                        EmittersPanel->SetEditingSystem(LoadedSystem);
                    }
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open Particle System");
        }
        ImGui::SameLine();
        if (IconSave && IconSave->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_SaveBtn", (void*)IconSave->GetShaderResourceView(), IconSizeVec))
            {
                if (EmittersPanel && EmittersPanel->GetEditingSystem())
                {
                    // Open save file dialog
                    OPENFILENAMEA ofn = {};
                    char szFile[260] = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Particle System Files\0*.particle\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = "Data\\ParticleSystems";
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = "particle";

                    if (GetSaveFileNameA(&ofn) == TRUE)
                    {
                        std::string narrowPath(szFile);
                        std::wstring widePath(narrowPath.begin(), narrowPath.end());

                        JSON ParticleJson = JSON::Make(JSON::Class::Object);
                        EmittersPanel->GetEditingSystem()->Serialize(false, ParticleJson);
                        FJsonSerializer::SaveJsonToFile(ParticleJson, widePath);
                    }
                }
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
            if (ImGui::ImageButton("##Cascade_RestartBtn", (void*)IconRestart->GetShaderResourceView(), IconSizeVec)) { }
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
    ImGui::TextUnformatted("Curve Editor");
    ImGui::Separator();
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
                // TODO: Pause particle simulation
            }
        }
        else
        {
            if (ImGui::MenuItem("Play"))
            {
                bIsPlaying = true;
                // TODO: Play particle simulation
            }
        }

        ImGui::Separator();

        // Realtime toggle
        if (ImGui::MenuItem("Realtime", nullptr, &bRealtime))
        {
            // TODO: Toggle realtime simulation
        }

        // Loop toggle
        if (ImGui::MenuItem("Loop", nullptr, &bLoopSimulation))
        {
            // TODO: Toggle loop simulation
        }

        ImGui::Separator();

        // AnimSpeed submenu
        if (ImGui::BeginMenu("AnimSpeed"))
        {
            if (ImGui::MenuItem("100%", nullptr, AnimSpeed == 1.0f))
            {
                AnimSpeed = 1.0f;
                // TODO: Set animation speed to 100%
            }
            if (ImGui::MenuItem("50%", nullptr, AnimSpeed == 0.5f))
            {
                AnimSpeed = 0.5f;
                // TODO: Set animation speed to 50%
            }
            if (ImGui::MenuItem("25%", nullptr, AnimSpeed == 0.25f))
            {
                AnimSpeed = 0.25f;
                // TODO: Set animation speed to 25%
            }
            if (ImGui::MenuItem("10%", nullptr, AnimSpeed == 0.1f))
            {
                AnimSpeed = 0.1f;
                // TODO: Set animation speed to 10%
            }

            ImGui::Separator();

            if (ImGui::MenuItem("200%", nullptr, AnimSpeed == 2.0f))
            {
                AnimSpeed = 2.0f;
                // TODO: Set animation speed to 200%
            }
            if (ImGui::MenuItem("500%", nullptr, AnimSpeed == 5.0f))
            {
                AnimSpeed = 5.0f;
                // TODO: Set animation speed to 500%
            }
            if (ImGui::MenuItem("1000%", nullptr, AnimSpeed == 10.0f))
            {
                AnimSpeed = 10.0f;
                // TODO: Set animation speed to 1000%
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

    UParticleSystem* EditingSystem = EmittersPanel->GetEditingSystem();
    // // Only override the preview when the editing system is actually populated
    // if (EditingSystem && EditingSystem->GetEmitterCount() > 0 && PreviewComp->Template != EditingSystem)
    if (EditingSystem && PreviewComp->Template != EditingSystem)
    {
        PreviewComp->SetTemplate(EditingSystem);
        PreviewComp->Restart();
    }
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
}

void SParticleSystemEditorWindow::RenderDetailsPanel(float width, float height)
{
    ImGui::TextUnformatted("Details");
    ImGui::Separator();

    if (!EmittersPanel)
    {
        ImGui::TextUnformatted("(No emitters panel)");
        return;
    }

    UParticleModule* SelectedModule = EmittersPanel->GetSelectedModule();
    if (!SelectedModule)
    {
        ImGui::TextUnformatted("Select a module to view properties");
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

    // Property label
    ImGui::Text("%s:", Prop->Name);

    switch (Prop->Type)
    {
    case EPropertyType::Bool:
    {
        bool* Value = Prop->GetValuePtr<bool>(Module);
        ImGui::Checkbox("##value", Value);
        break;
    }
    case EPropertyType::Int32:
    {
        int32* Value = Prop->GetValuePtr<int32>(Module);
        ImGui::DragInt("##value", Value, 1.0f, (int)Prop->MinValue, (int)Prop->MaxValue);
        break;
    }
    case EPropertyType::Float:
    {
        float* Value = Prop->GetValuePtr<float>(Module);
        ImGui::DragFloat("##value", Value, 0.01f, Prop->MinValue, Prop->MaxValue);
        break;
    }
    case EPropertyType::FVector:
    {
        FVector* Value = Prop->GetValuePtr<FVector>(Module);
        ImGui::DragFloat3("##value", &Value->X, 0.1f);
        break;
    }
    case EPropertyType::FLinearColor:
    {
        FLinearColor* Value = Prop->GetValuePtr<FLinearColor>(Module);
        ImGui::ColorEdit4("##value", &Value->R);
        break;
    }
    case EPropertyType::FString:
    {
        FString* Value = Prop->GetValuePtr<FString>(Module);
        char buffer[256];
        strncpy_s(buffer, Value->c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText("##value", buffer, sizeof(buffer)))
        {
            *Value = FString(buffer);
        }
        break;
    }
    default:
        ImGui::TextUnformatted("(Unsupported type)");
        break;
    }

    ImGui::PopID();
}
