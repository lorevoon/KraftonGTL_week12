#include "pch.h"
#include "SCascadeEmittersPanel.h"
#include "ImGui/imgui.h"

#include "Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleRequired.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleSpawn.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleLifetime.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleLocation.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleVelocity.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleColor.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleSize.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleSizeScaleBySpeed.h"
#include "Source/Runtime/Engine/Particles/Modules/TypeData/ParticleModuleTypeDataMesh.h"
#include "Source/Runtime/Engine/Particles/Modules/TypeData/ParticleModuleTypeDataBeam.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleBeamNoise.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleCollision.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleAcceleration.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleEventGenerator.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ParticleModuleTypeDataRibbon.h"
#include "Source/Runtime/Engine/Particles/Modules/ParticleModuleSpawnPerUnit.h"

void SCascadeEmittersPanel::SetEditingSystem(UParticleSystem* InSystem)
{
    EditingSystem = InSystem;
    // Reset editor states when changing systems
    EmitterEditorStates.Empty();
    EnsureEditorStates();
}

void SCascadeEmittersPanel::EnsureEditingSystem()
{
    if (!EditingSystem)
    {
        EditingSystem = NewObject<UParticleSystem>();
        EditingSystem->SystemName = "NewParticleSystem";
    }
    EnsureEditorStates();
}

void SCascadeEmittersPanel::EnsureEditorStates()
{
    if (!EditingSystem)
        return;

    int32 EmitterCount = EditingSystem->GetEmitterCount();
    // Grow the array if needed
    while (EmitterEditorStates.Num() < EmitterCount)
    {
        EmitterEditorStates.Add(FEmitterEditorState());
    }
    // Shrink if emitters were removed
    if (EmitterEditorStates.Num() > EmitterCount)
    {
        EmitterEditorStates.resize(EmitterCount);
    }
}

FEmitterEditorState& SCascadeEmittersPanel::GetEmitterEditorState(int32 EmitterIndex)
{
    EnsureEditorStates();
    static FEmitterEditorState DefaultState;
    if (EmitterIndex >= 0 && EmitterIndex < EmitterEditorStates.Num())
    {
        return EmitterEditorStates[EmitterIndex];
    }
    return DefaultState;
}

bool SCascadeEmittersPanel::IsEmitterVisibleInEditor(int32 EmitterIndex) const
{
    if (EmitterIndex < 0 || EmitterIndex >= EmitterEditorStates.Num())
        return true;

    const FEmitterEditorState& State = EmitterEditorStates[EmitterIndex];

    // Check if any emitter has solo enabled
    bool bAnySolo = HasAnySoloEmitter();

    if (bAnySolo)
    {
        // If any emitter is solo'd, only show solo'd emitters
        return State.bIsSolo && State.bIsVisible;
    }
    else
    {
        // Normal visibility check
        return State.bIsVisible;
    }
}

bool SCascadeEmittersPanel::HasAnySoloEmitter() const
{
    for (const FEmitterEditorState& State : EmitterEditorStates)
    {
        if (State.bIsSolo)
            return true;
    }
    return false;
}

static void DrawCascadePanelHeader(const char* Title)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float fullW = ImGui::GetContentRegionAvail().x;
    const float headerH = 28.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 bg = ImGui::GetColorU32(ImVec4(0.14f, 0.14f, 0.18f, 1.0f));
    ImU32 top = ImGui::GetColorU32(ImVec4(0.22f, 0.22f, 0.28f, 1.0f));
    ImU32 border = ImGui::GetColorU32(ImVec4(0.35f, 0.35f, 0.40f, 1.0f));

    dl->AddRectFilled(pos, ImVec2(pos.x + fullW, pos.y + headerH), bg, 5.0f);
    dl->AddLine(ImVec2(pos.x, pos.y), ImVec2(pos.x + fullW, pos.y), top, 1.0f);
    dl->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + fullW, pos.y + headerH), border, 5.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + 10.0f, pos.y + 5.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 1.00f, 1.0f));
    ImGui::TextUnformatted(Title);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + headerH + 6.0f));
}

void SCascadeEmittersPanel::Render(float width, float height)
{
    EnsureEditingSystem();

    // Header row styled like a panel title
    DrawCascadePanelHeader("Emitters");

    // Reset frame-local click tracking
    bClickedOnItemThisFrame = false;

    // Horizontally scrollable canvas for vertical emitter stacks
    ImGui::BeginChild("Cascade_Emitters_Canvas", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    const float columnWidth = 180.0f;
    const float headerHeight = 50.0f;
    const float thumbnailSize = 36.0f;
    const float moduleHeight = 24.0f;
    const float columnSpacing = 10.0f;
    const float iconSize = 16.0f;

    int count = EditingSystem->GetEmitterCount();
    for (int i = 0; i < count; ++i)
    {
        UParticleEmitter* Emitter = EditingSystem->GetEmitter(i);
        FString Name = Emitter ? Emitter->EmitterName : FString("<Unnamed>");

        if (i > 0)
            ImGui::SameLine(0.0f, columnSpacing);

        ImGui::PushID(i);
        ImGui::BeginGroup();

        // ===== HEADER SECTION =====
        bool selected = (i == SelectedEmitterIndex);

        // Header background
        ImVec4 headerBgColor = selected ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f) : ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, headerBgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
        ImGui::BeginChild("EmitterHeader", ImVec2(columnWidth, headerHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Left side: Name and control buttons
        ImGui::BeginGroup();

        // First row: Emitter name
        ImGui::TextUnformatted(Name.c_str());

        // Second row: Control icons (horizontal layout)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1));  // Wider padding, minimal vertical

        const float iconButtonWidth = 20.0f;
        const float iconButtonHeight = 20.0f;
        FEmitterEditorState& EditorState = GetEmitterEditorState(i);

        // Visibility toggle (V)
        ImVec4 visButtonColor = EditorState.bIsVisible ? ImVec4(0.2f, 0.5f, 0.2f, 1.0f) : ImVec4(0.5f, 0.2f, 0.2f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, visButtonColor);
        if (ImGui::Button("V##vis", ImVec2(iconButtonWidth, iconButtonHeight)))
        {
            EditorState.bIsVisible = !EditorState.bIsVisible;
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle Visibility (%s)", EditorState.bIsVisible ? "Visible" : "Hidden");
        }

        ImGui::SameLine();

        // Render mode toggle (R) - cycles through modes
        const char* renderModeLabels[] = { "N", "P", "X", "-" }; // Normal, Points, Cross, None
        const char* renderModeNames[] = { "Normal", "Points", "Cross", "None" };
        int renderModeIndex = static_cast<int>(EditorState.RenderMode);
        ImVec4 renderButtonColor = (EditorState.RenderMode == EEmitterEditorRenderMode::Normal)
            ? ImVec4(0.3f, 0.3f, 0.3f, 1.0f)
            : ImVec4(0.4f, 0.3f, 0.2f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, renderButtonColor);
        if (ImGui::Button(renderModeLabels[renderModeIndex], ImVec2(iconButtonWidth, iconButtonHeight)))
        {
            // Cycle to next render mode
            renderModeIndex = (renderModeIndex + 1) % 4;
            EditorState.RenderMode = static_cast<EEmitterEditorRenderMode>(renderModeIndex);
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Render Mode: %s (click to cycle)", renderModeNames[static_cast<int>(EditorState.RenderMode)]);
        }

        ImGui::SameLine();

        // Solo toggle (S)
        ImVec4 soloButtonColor = EditorState.bIsSolo ? ImVec4(0.6f, 0.5f, 0.1f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, soloButtonColor);
        if (ImGui::Button("S##solo", ImVec2(iconButtonWidth, iconButtonHeight)))
        {
            EditorState.bIsSolo = !EditorState.bIsSolo;
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Solo Mode (%s)", EditorState.bIsSolo ? "Solo" : "Off");
        }

        ImGui::PopStyleVar(3);

        ImGui::EndGroup();

        // Right side: Particle sprite preview thumbnail
        ImGui::SameLine();
        float rightSideX = columnWidth - thumbnailSize - 8.0f;
        ImGui::SetCursorPosX(rightSideX);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        float thumbnailDisplaySize = 36.0f;
        ImGui::Button("##thumbnail", ImVec2(thumbnailDisplaySize, thumbnailDisplaySize));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        // Draw a simple particle sprite representation in the thumbnail
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 thumbMin = ImGui::GetItemRectMin();
        ImVec2 thumbMax = ImGui::GetItemRectMax();
        ImVec2 center = ImVec2((thumbMin.x + thumbMax.x) * 0.5f, (thumbMin.y + thumbMax.y) * 0.5f);
        drawList->AddCircleFilled(center, 6.0f, IM_COL32(255, 255, 255, 200), 8);
        drawList->AddCircleFilled(center, 4.0f, IM_COL32(255, 200, 100, 255), 8);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Check if this item (the header child) is being interacted with
        bool isItemActive = ImGui::IsItemActive();
        bool isItemHovered = ImGui::IsItemHovered();

        // Click on header to select
        if (ImGui::IsItemClicked())
        {
            SelectedEmitterIndex = i;
            SelectedModule = nullptr; // Deselect module when clicking emitter header
            bClickedOnItemThisFrame = true;
        }

        // Drag source for emitter reordering (only if item is active)
        if (isItemActive && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("EMITTER_REORDER", &i, sizeof(int32));
            ImGui::Text("Emitter: %s", Name.c_str());
            ImGui::EndDragDropSource();
        }

        // Drop target for emitter reordering
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EMITTER_REORDER"))
            {
                int32 sourceIndex = *(const int32*)payload->Data;
                EditingSystem->SwapEmitters(sourceIndex, i);
                if (EditingSystem) EditingSystem->bIsDirty = true;
                // Update selected index if necessary
                if (SelectedEmitterIndex == sourceIndex)
                    SelectedEmitterIndex = i;
                else if (SelectedEmitterIndex == i)
                    SelectedEmitterIndex = sourceIndex;
            }
            ImGui::EndDragDropTarget();
        }

        // Popup on header
        if (ImGui::BeginPopupContextItem("EmitterHeaderPopup"))
        {
            if (ImGui::MenuItem("Delete Emitter"))
            {
                UParticleEmitter* ToRemove = EditingSystem->GetEmitter(i);
                EditingSystem->RemoveEmitter(ToRemove);
                if (EditingSystem) EditingSystem->bIsDirty = true;
                if (SelectedEmitterIndex >= EditingSystem->GetEmitterCount())
                    SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
                ImGui::EndPopup();
                ImGui::EndGroup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        // ===== MODULES SECTION =====
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild("EmitterModules", ImVec2(columnWidth, 0), true, ImGuiWindowFlags_NoScrollbar);

        UParticleLODLevel* LOD0 = Emitter ? Emitter->GetDefaultLODLevel() : nullptr;
        if (LOD0)
        {
            // Required module - YELLOW, NO CHECKBOX (not draggable)
            if (LOD0->RequiredModule)
            {
                const char* reqName = LOD0->RequiredModule->ModuleName.c_str();
                if (reqName)
                {
                    RenderModuleCard(LOD0->RequiredModule,
                                   LOD0,
                                   -1, // -1 indicates not draggable (required module)
                                   reqName,
                                   ImVec4(0.6f, 0.5f, 0.2f, 1.0f), // Yellow
                                   columnWidth, moduleHeight, false); // No checkbox
                }
            }

            // TypeData module - PURPLE (special module, not draggable)
            if (LOD0->TypeDataModule)
            {
                const char* typeName = LOD0->TypeDataModule->ModuleName.c_str();
                if (typeName)
                {
                    RenderModuleCard(LOD0->TypeDataModule,
                                   LOD0,
                                   -2, // -2 indicates TypeData module
                                   typeName,
                                   ImVec4(0.4f, 0.2f, 0.5f, 1.0f), // Purple
                                   columnWidth, moduleHeight, true); // With checkbox
                }
            }

            // EventGenerator module - TEAL (event-related)
            if (LOD0->EventGenerator)
            {
                const char* eventGenName = LOD0->EventGenerator->ModuleName.c_str();
                if (eventGenName)
                {
                    RenderModuleCard(LOD0->EventGenerator,
                                   LOD0,
                                   -3, // -3 indicates EventGenerator module
                                   eventGenName,
                                   ImVec4(0.2f, 0.5f, 0.6f, 1.0f), // Teal
                                   columnWidth, moduleHeight, true); // With checkbox
                }
            }

            // Other modules
            const TArray<UParticleModule*>& Modules = LOD0->Modules;
            for (int m = 0; m < Modules.Num(); ++m)
            {
                if (UParticleModule* Mod = Modules[m])
                {
                    const char* modName = Mod->ModuleName.c_str();
                    if (modName)
                    {
                        ImVec4 moduleColor = GetModuleColor(Mod->ModuleName);
                        RenderModuleCard(Mod, LOD0, m, modName, moduleColor, columnWidth, moduleHeight, true); // With checkbox
                    }
                }
            }
        }

        // Empty area popup - Add Module context menu
        if (ImGui::BeginPopupContextWindow("EmitterColumnEmpty", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            ImGui::TextUnformatted("Add Module");
            ImGui::Separator();

            if (ImGui::MenuItem("Spawn"))
            {
                UParticleModuleSpawn* NewModule = NewObject<UParticleModuleSpawn>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Spawn";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Lifetime"))
            {
                UParticleModuleLifetime* NewModule = NewObject<UParticleModuleLifetime>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Lifetime";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Initial Location"))
            {
                UParticleModuleLocation* NewModule = NewObject<UParticleModuleLocation>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Location";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Initial Velocity"))
            {
                UParticleModuleVelocity* NewModule = NewObject<UParticleModuleVelocity>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Velocity";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Initial Size"))
            {
                UParticleModuleSize* NewModule = NewObject<UParticleModuleSize>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Size";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Color Over Life"))
            {
                UParticleModuleColor* NewModule = NewObject<UParticleModuleColor>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Color";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Size Scale By Speed"))
            {
                UParticleModuleSizeScaleBySpeed* NewModule = NewObject<UParticleModuleSizeScaleBySpeed>();
                if (NewModule)
                {
                    NewModule->ModuleName = "SizeScaleBySpeed";
                    LOD0->AddModule(NewModule);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Collision"))
            {
                UParticleModuleCollision* NewModule = NewObject<UParticleModuleCollision>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Collision";
                    LOD0->AddModule(NewModule);
                }
            }

            if (ImGui::MenuItem("Acceleration"))
            {
                UParticleModuleAcceleration* NewModule = NewObject<UParticleModuleAcceleration>();
                if (NewModule)
                {
                    NewModule->ModuleName = "Acceleration";
                    LOD0->AddModule(NewModule);
                }
            }

            if (ImGui::MenuItem("Event Generator"))
            {
                UParticleModuleEventGenerator* NewModule = NewObject<UParticleModuleEventGenerator>();
                if (NewModule)
                {
                    NewModule->ModuleName = "EventGenerator";
                    LOD0->EventGenerator = NewModule;
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled("TypeData Modules");

            if (ImGui::MenuItem("TypeData Mesh"))
            {
                UParticleModuleTypeDataMesh* NewModule = NewObject<UParticleModuleTypeDataMesh>();
                if (NewModule)
                {
                    NewModule->ModuleName = "TypeData Mesh";
                    LOD0->TypeDataModule = NewModule;
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("TypeData Beam"))
            {
                UParticleModuleTypeDataBeam* NewModule = NewObject<UParticleModuleTypeDataBeam>();
                if (NewModule)
                {
                    NewModule->ModuleName = "TypeData Beam";
                    LOD0->TypeDataModule = NewModule;
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);

        ImGui::EndGroup();
        ImGui::PopID();
    }

    // Right-click empty canvas to add new emitter
    if (ImGui::BeginPopupContextWindow("EmittersEmptyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
    {
        ImGui::TextDisabled("New Emitter");
        ImGui::Separator();

        if (ImGui::MenuItem("Sprite Emitter"))
        {
            if (UParticleEmitter* NewEmitter = CreateDefaultSpriteEmitter())
            {
                EditingSystem->AddEmitter(NewEmitter);
                SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
                if (EditingSystem) EditingSystem->bIsDirty = true;
            }
        }

        if (ImGui::MenuItem("Mesh Emitter"))
        {
            if (UParticleEmitter* NewEmitter = CreateDefaultMeshEmitter())
            {
                EditingSystem->AddEmitter(NewEmitter);
                SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
                if (EditingSystem) EditingSystem->bIsDirty = true;
            }
        }

        if (ImGui::MenuItem("Beam Emitter"))
        {
            if (UParticleEmitter* NewEmitter = CreateDefaultBeamEmitter())
            {
                EditingSystem->AddEmitter(NewEmitter);
                SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
                if (EditingSystem) EditingSystem->bIsDirty = true;
            }
        }

        if (ImGui::MenuItem("Ribbon Emitter"))
        {
            if (UParticleEmitter* NewEmitter = CreateDefaultRibbonEmitter())
            {
                EditingSystem->AddEmitter(NewEmitter);
                SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
                if (EditingSystem) EditingSystem->bIsDirty = true;
            }
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();

    // Detect clicks on empty space to deselect both module and emitter
    // This allows clicking blank area to show UParticleSystem properties
    // We check after EndChild so that all item hover states are finalized
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // If we didn't click on any emitter/module item, deselect everything
        if (!bClickedOnItemThisFrame && !ImGui::IsAnyItemActive())
        {
            SelectedModule = nullptr;
            SelectedEmitterIndex = -1;
        }
    }

    // Keyboard navigation for reordering emitters (like Unreal Engine)
    if (SelectedEmitterIndex >= 0 && SelectedEmitterIndex < EditingSystem->GetEmitterCount())
    {
        // Left arrow key - move emitter to the left
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && SelectedEmitterIndex > 0)
        {
            EditingSystem->SwapEmitters(SelectedEmitterIndex, SelectedEmitterIndex - 1);
            SelectedEmitterIndex--;
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
        // Right arrow key - move emitter to the right
        else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && SelectedEmitterIndex < EditingSystem->GetEmitterCount() - 1)
        {
            EditingSystem->SwapEmitters(SelectedEmitterIndex, SelectedEmitterIndex + 1);
            SelectedEmitterIndex++;
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
    }
}

UParticleEmitter* SCascadeEmittersPanel::CreateDefaultSpriteEmitter()
{
    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
    if (!Emitter)
        return nullptr;

    // Basic defaults
    Emitter->EmitterName = "Sprite Emitter";
    Emitter->MaxParticleCount = 1000;

    // Create default LOD 0
    UParticleLODLevel* LOD0 = NewObject<UParticleLODLevel>();
    if (!LOD0)
        return nullptr;

    LOD0->Level = 0;
    LOD0->DistanceThreshold = 0.0f;

    // Required module
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    if (Required)
    {
        Required->ModuleName = "Required";
        LOD0->RequiredModule = Required;
    }

    // Spawn module
    if (UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>())
    {
        Spawn->ModuleName = "Spawn";
        LOD0->AddModule(Spawn);
    }
    // Lifetime
    if (UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>())
    {
        Lifetime->ModuleName = "Lifetime";
        LOD0->AddModule(Lifetime);
    }
    // Initial Location
    if (UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>())
    {
        Location->ModuleName = "Location";
        LOD0->AddModule(Location);
    }
    // Initial Velocity
    if (UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>())
    {
        Velocity->ModuleName = "Velocity";
        LOD0->AddModule(Velocity);
    }
    // Initial Size
    if (UParticleModuleSize* Size = NewObject<UParticleModuleSize>())
    {
        Size->ModuleName = "Size";
        LOD0->AddModule(Size);
    }
    // Initial Color / Color Over Life
    if (UParticleModuleColor* Color = NewObject<UParticleModuleColor>())
    {
        Color->ModuleName = "Color";
        LOD0->AddModule(Color);
    }

    Emitter->AddLODLevel(LOD0);
    return Emitter;
}

UParticleEmitter* SCascadeEmittersPanel::CreateDefaultMeshEmitter()
{
    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
    if (!Emitter)
        return nullptr;

    Emitter->EmitterName = "Mesh Emitter";
    Emitter->MaxParticleCount = 100;

    UParticleLODLevel* LOD0 = NewObject<UParticleLODLevel>();
    if (!LOD0)
        return nullptr;

    LOD0->Level = 0;
    LOD0->DistanceThreshold = 0.0f;

    // Required module
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    if (Required)
    {
        Required->ModuleName = "Required";
        Required->SpawnRate = 5.0f; // Lower spawn rate for mesh particles
        LOD0->RequiredModule = Required;
    }

    // TypeData Mesh module
    UParticleModuleTypeDataMesh* TypeDataMesh = NewObject<UParticleModuleTypeDataMesh>();
    if (TypeDataMesh)
    {
        TypeDataMesh->ModuleName = "TypeData Mesh";
        LOD0->TypeDataModule = TypeDataMesh;
    }

    // Spawn
    if (UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>())
    {
        Spawn->ModuleName = "Spawn";
        LOD0->AddModule(Spawn);
    }
    // Lifetime
    if (UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>())
    {
        Lifetime->ModuleName = "Lifetime";
        Lifetime->LifetimeMin = 2.0f;
        Lifetime->LifetimeMax = 3.0f;
        LOD0->AddModule(Lifetime);
    }
    // Location
    if (UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>())
    {
        Location->ModuleName = "Location";
        LOD0->AddModule(Location);
    }
    // Velocity
    if (UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>())
    {
        Velocity->ModuleName = "Velocity";
        Velocity->StartVelocityMin = FVector(-50.f, 50.f, -50.f);
        Velocity->StartVelocityMax = FVector(50.f, 150.f, 50.f);
        LOD0->AddModule(Velocity);
    }
    // Size
    if (UParticleModuleSize* Size = NewObject<UParticleModuleSize>())
    {
        Size->ModuleName = "Size";
        Size->StartSizeMin = FVector(0.5f, 0.5f, 0.5f);
        Size->StartSizeMax = FVector(1.0f, 1.0f, 1.0f);
        LOD0->AddModule(Size);
    }
    // Color
    if (UParticleModuleColor* Color = NewObject<UParticleModuleColor>())
    {
        Color->ModuleName = "Color";
        LOD0->AddModule(Color);
    }

    Emitter->AddLODLevel(LOD0);
    return Emitter;
}

UParticleEmitter* SCascadeEmittersPanel::CreateDefaultBeamEmitter()
{
    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
    if (!Emitter)
        return nullptr;

    Emitter->EmitterName = "Beam Emitter";
    Emitter->MaxParticleCount = 10;

    UParticleLODLevel* LOD0 = NewObject<UParticleLODLevel>();
    if (!LOD0)
        return nullptr;

    LOD0->Level = 0;
    LOD0->DistanceThreshold = 0.0f;

    // Required module
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    if (Required)
    {
        Required->ModuleName = "Required";
        Required->SpawnRate = 2.0f; // Low spawn rate for beams
        LOD0->RequiredModule = Required;
    }

    // TypeData Beam module
    UParticleModuleTypeDataBeam* TypeDataBeam = NewObject<UParticleModuleTypeDataBeam>();
    if (TypeDataBeam)
    {
        TypeDataBeam->ModuleName = "TypeData Beam";
        TypeDataBeam->Segments = 8;
        TypeDataBeam->Width = 0.1f;      // 10cm
        TypeDataBeam->Length = 5.0f;     // 5m
        LOD0->TypeDataModule = TypeDataBeam;

        // BeamNoise 모듈 추가
        UParticleModuleBeamNoise* BeamNoiseModule = NewObject<UParticleModuleBeamNoise>();
        BeamNoiseModule->NoiseRange = FVector(0.3f, 0.3f, 0.0f);  // 30cm
        BeamNoiseModule->NoiseLockTime = 0.1f;
        BeamNoiseModule->bSmooth = true;
        LOD0->AddModule(BeamNoiseModule);
    }

    // Spawn
    if (UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>())
    {
        Spawn->ModuleName = "Spawn";
        LOD0->AddModule(Spawn);
    }
    // Lifetime
    if (UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>())
    {
        Lifetime->ModuleName = "Lifetime";
        Lifetime->LifetimeMin = 1.0f;
        Lifetime->LifetimeMax = 2.0f;
        LOD0->AddModule(Lifetime);
    }
    // Location
    if (UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>())
    {
        Location->ModuleName = "Location";
        LOD0->AddModule(Location);
    }
    // Velocity - beam direction
    if (UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>())
    {
        Velocity->ModuleName = "Velocity";
        Velocity->StartVelocityMin = FVector(100.f, 0.f, 0.f);
        Velocity->StartVelocityMax = FVector(200.f, 50.f, 0.f);
        LOD0->AddModule(Velocity);
    }
    // Size
    if (UParticleModuleSize* Size = NewObject<UParticleModuleSize>())
    {
        Size->ModuleName = "Size";
        LOD0->AddModule(Size);
    }
    // Color
    if (UParticleModuleColor* Color = NewObject<UParticleModuleColor>())
    {
        Color->ModuleName = "Color";
        Color->StartColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f); // Blue-ish
        Color->EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f); // Fade to transparent
        LOD0->AddModule(Color);
    }

    Emitter->AddLODLevel(LOD0);
    return Emitter;
}

UParticleEmitter* SCascadeEmittersPanel::CreateDefaultRibbonEmitter()
{
    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
    if (!Emitter)
        return nullptr;

    Emitter->EmitterName = "Ribbon Emitter";
    Emitter->MaxParticleCount = 200; // 리본 궤적을 위해 많은 파티클

    UParticleLODLevel* LOD0 = NewObject<UParticleLODLevel>();
    if (!LOD0)
        return nullptr;

    LOD0->Level = 0;
    LOD0->DistanceThreshold = 0.0f;

    // Required module
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    if (Required)
    {
        Required->ModuleName = "Required";
        Required->SpawnRate = 0.0f; // SpawnPerUnit 모듈이 거리 기반으로 스폰하므로 0
        Required->bUseLocalSpace = false; // World Space 사용
        LOD0->RequiredModule = Required;
    }

    // Ribbon TypeData module (RibbonParticleActor와 동일한 설정)
    UParticleModuleTypeDataRibbon* RibbonModule = NewObject<UParticleModuleTypeDataRibbon>();
    if (RibbonModule)
    {
        RibbonModule->ModuleName = "Ribbon";
        RibbonModule->Width = 2.0f;                           // 리본 너비 2m
        RibbonModule->MaxParticleInTrailCount = 200;          // 최대 200개 파티클로 궤적 구성 (긴 궤적)
        RibbonModule->TilingDistance = 5.0f;                  // 5m마다 UV 타일링
        RibbonModule->RenderAxis = ERibbonRenderAxis::CameraUp;  // 카메라를 향함
        RibbonModule->DistanceTessellationStepSize = 0.5f;    // 0.5m마다 테셀레이션
        RibbonModule->MaxTessellationBetweenParticles = 25;   // 최대 25개 보간
        RibbonModule->bEnableTangentDiffInterpScale = true;   // Tangent 각도로 추가 세분화
        LOD0->TypeDataModule = RibbonModule;  // TypeDataModule로 설정
    }

    // SpawnPerUnit 모듈 (거리 기반 스폰 - Trail 필수)
    if (UParticleModuleSpawnPerUnit* SpawnPerUnit = NewObject<UParticleModuleSpawnPerUnit>())
    {
        SpawnPerUnit->ModuleName = "SpawnPerUnit";
        SpawnPerUnit->SpawnPerUnit = 20.0f;           // 단위당 20개
        SpawnPerUnit->UnitScalar = 1.0f;              // 1m당 (더 조밀하게)
        SpawnPerUnit->MaxFrameDistance = 200;         // 프레임당 최대 200개
        SpawnPerUnit->bSpawnOnMovementStart = true;   // 첫 이동 시 스폰
        LOD0->AddModule(SpawnPerUnit);
    }

    // Lifetime (긴 수명으로 궤적 유지 - RibbonParticleActor와 동일)
    if (UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>())
    {
        Lifetime->ModuleName = "Lifetime";
        Lifetime->LifetimeMin = 5.0f;
        Lifetime->LifetimeMax = 10.0f;
        LOD0->AddModule(Lifetime);
    }
    // Location
    if (UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>())
    {
        Location->ModuleName = "Location";
        LOD0->AddModule(Location);
    }
    // Velocity (궤적 생성을 위한 움직임)
    if (UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>())
    {
        Velocity->ModuleName = "Velocity";
        Velocity->StartVelocityMin = FVector(2.0f, 0.f, 0.5f);
        Velocity->StartVelocityMax = FVector(2.0f, 0.f, 1.0f);
        LOD0->AddModule(Velocity);
    }
    // Size
    if (UParticleModuleSize* Size = NewObject<UParticleModuleSize>())
    {
        Size->ModuleName = "Size";
        LOD0->AddModule(Size);
    }
    // Color
    if (UParticleModuleColor* Color = NewObject<UParticleModuleColor>())
    {
        Color->ModuleName = "Color";
        Color->StartColor = FLinearColor(1.0f, 0.5f, 0.2f, 1.0f); // 주황색
        Color->EndColor = FLinearColor(1.0f, 0.8f, 0.2f, 0.0f);   // 노란색으로 페이드
        LOD0->AddModule(Color);
    }

    Emitter->AddLODLevel(LOD0);
    return Emitter;
}

void SCascadeEmittersPanel::RenderModuleCard(UParticleModule* module, UParticleLODLevel* parentLOD, int32 moduleIndex, const char* moduleName, const ImVec4& backgroundColor, float width, float height, bool showCheckbox)
{
    if (!module)
    {
        // Safety check: render a placeholder if module is null
        ImGui::TextUnformatted("(Invalid module)");
        return;
    }

    // Use module pointer as unique ID
    ImGui::PushID(module);

    // Highlight if this module is selected
    bool isSelected = (module == SelectedModule);
    ImVec4 finalBackgroundColor = backgroundColor;
    if (isSelected)
    {
        // Add a bright border/highlight effect for selected module
        finalBackgroundColor = ImVec4(backgroundColor.x * 1.3f, backgroundColor.y * 1.3f, backgroundColor.z * 1.3f, backgroundColor.w);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, finalBackgroundColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(finalBackgroundColor.x * 1.1f, finalBackgroundColor.y * 1.1f, finalBackgroundColor.z * 1.1f, finalBackgroundColor.w));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(finalBackgroundColor.x * 0.9f, finalBackgroundColor.y * 0.9f, finalBackgroundColor.z * 0.9f, finalBackgroundColor.w));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));

    // Create a horizontal layout with checkbox (if needed) and button
    ImGui::BeginGroup();

    if (showCheckbox)
    {
        // Render checkbox for enable/disable
        bool enabled = module->bEnabled;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        if (ImGui::Checkbox("##enabled", &enabled))
        {
            module->bEnabled = enabled;
            if (EditingSystem) EditingSystem->bIsDirty = true;
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();

        // Button takes remaining width
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        if (ImGui::Button(moduleName, ImVec2(width - 30, height)))
        {
            SelectedModule = module;
            bClickedOnItemThisFrame = true;
        }
        ImGui::PopStyleVar();

        // Context menu on the button
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextUnformatted(moduleName);
            ImGui::Separator();

            // Toggle enable/disable
            bool isEnabled = module->bEnabled;
            if (ImGui::MenuItem("Enable/Disable", nullptr, &isEnabled))
            {
                module->bEnabled = isEnabled;
            }

            if (ImGui::MenuItem("Delete"))
            {
                if (parentLOD)
                {
                    // Clear selection if we're deleting the selected module
                    if (SelectedModule == module)
                    {
                        SelectedModule = nullptr;
                    }
                    // TypeData module (moduleIndex == -2) has special handling
                    if (moduleIndex == -2)
                    {
                        parentLOD->TypeDataModule = nullptr;
                    }
                    // EventGenerator module (moduleIndex == -3) has special handling
                    else if (moduleIndex == -3)
                    {
                        parentLOD->EventGenerator = nullptr;
                    }
                    else
                    {
                        parentLOD->RemoveModule(module);
                    }
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                if (parentLOD && module)
                {
                    // Duplicate the module
                    UParticleModule* ClonedModule = static_cast<UParticleModule*>(module->Duplicate());
                    if (ClonedModule)
                    {
                        // Add it to the parent LOD level
                        parentLOD->AddModule(ClonedModule);
                        if (EditingSystem) EditingSystem->bIsDirty = true;
                    }
                }
            }

            ImGui::EndPopup();
        }
    }
    else
    {
        // No checkbox for Required module, just the button with left padding
        char buttonLabel[128];
        sprintf_s(buttonLabel, "     %s", moduleName);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        if (ImGui::Button(buttonLabel, ImVec2(width, height)))
        {
            SelectedModule = module;
            bClickedOnItemThisFrame = true;
        }
        ImGui::PopStyleVar();

        // Context menu on the button (Required module)
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextUnformatted(moduleName);
            ImGui::Separator();

            // Required module cannot be deleted
            ImGui::TextDisabled("(Required modules cannot be deleted)");

            if (ImGui::MenuItem("Duplicate"))
            {
                if (parentLOD && module)
                {
                    // Duplicate the required module and add it as a regular module
                    UParticleModule* ClonedModule = static_cast<UParticleModule*>(module->Duplicate());
                    if (ClonedModule)
                    {
                        // Add it to the parent LOD level as a regular module
                        parentLOD->AddModule(ClonedModule);
                        if (EditingSystem) EditingSystem->bIsDirty = true;
                    }
                }
            }

            ImGui::EndPopup();
        }
    }

    ImGui::EndGroup();

    // Check if the group item is being interacted with
    bool isItemActive = ImGui::IsItemActive();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    // Drag-and-drop for module reordering (only for non-required modules)
    if (moduleIndex >= 0 && showCheckbox && isItemActive)
    {
        // Drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("MODULE_REORDER", &moduleIndex, sizeof(int32));
            ImGui::Text("Module: %s", moduleName);
            ImGui::EndDragDropSource();
        }
    }

    // Drop target (can accept drops even when not active)
    if (moduleIndex >= 0 && showCheckbox)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODULE_REORDER"))
            {
                int32 sourceIndex = *(const int32*)payload->Data;
                if (parentLOD)
                {
                    parentLOD->SwapModules(sourceIndex, moduleIndex);
                    if (EditingSystem) EditingSystem->bIsDirty = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::PopID(); // Pop the unique ID
}

ImVec4 SCascadeEmittersPanel::GetModuleColor(const FString& moduleName)
{
    // Color scheme matching Unreal Cascade
    // Required = yellow (handled separately)
    // Spawn = red
    // EventGenerator = teal (handled separately)
    // Everything else = grey
    if (moduleName == "Spawn")
        return ImVec4(0.55f, 0.2f, 0.2f, 1.0f); // Red for Spawn
    else
        return ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Grey for all other modules (including Collision)
}

