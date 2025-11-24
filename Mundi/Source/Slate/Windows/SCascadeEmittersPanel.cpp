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

void SCascadeEmittersPanel::EnsureEditingSystem()
{
    if (!EditingSystem)
    {
        EditingSystem = NewObject<UParticleSystem>();
        EditingSystem->SystemName = "NewParticleSystem";
    }
}

void SCascadeEmittersPanel::Render(float width, float height)
{
    EnsureEditingSystem();

    // Header row with optional add button
    ImGui::TextUnformatted("Emitters");
    ImGui::SameLine();
    if (ImGui::SmallButton("+ New"))
    {
        if (UParticleEmitter* NewEmitter = CreateDefaultSpriteEmitter())
        {
            EditingSystem->AddEmitter(NewEmitter);
            SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
        }
    }
    ImGui::Separator();

    // Horizontally scrollable canvas for vertical emitter stacks
    ImGui::BeginChild("Cascade_Emitters_Canvas", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    const float columnWidth = 180.0f;
    const float headerHeight = 32.0f;
    const float thumbnailSize = 32.0f;
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

        // Header background (left part with name and icons)
        ImVec4 headerBgColor = selected ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f) : ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, headerBgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        ImGui::BeginChild("EmitterHeader", ImVec2(columnWidth, headerHeight), true);

        // Draw icon boxes (visibility, render mode, solo mode)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        if (ImGui::SmallButton("[]")) {} // Visibility checkbox
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Visibility");
        ImGui::SameLine();

        if (ImGui::SmallButton("[]")) {} // Render mode
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render Mode");
        ImGui::SameLine();

        if (ImGui::SmallButton("[]")) {} // Solo mode
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Solo Mode");

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        // Emitter number badge on the right
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        char badgeLabel[8];
        sprintf_s(badgeLabel, " %d ", i);
        ImGui::Button(badgeLabel, ImVec2(thumbnailSize, headerHeight - 4));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Click on header to select
        if (ImGui::IsItemClicked())
        {
            SelectedEmitterIndex = i;
        }

        // Popup on header
        if (ImGui::BeginPopupContextItem("EmitterHeaderPopup"))
        {
            if (ImGui::MenuItem("Delete Emitter"))
            {
                UParticleEmitter* ToRemove = EditingSystem->GetEmitter(i);
                EditingSystem->RemoveEmitter(ToRemove);
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
            // Required module - YELLOW, NO CHECKBOX
            if (LOD0->RequiredModule)
            {
                const char* reqName = LOD0->RequiredModule->ModuleName.c_str();
                if (reqName)
                {
                    RenderModuleCard(LOD0->RequiredModule,
                                   reqName,
                                   ImVec4(0.6f, 0.5f, 0.2f, 1.0f), // Yellow
                                   columnWidth, moduleHeight, false); // No checkbox
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
                        RenderModuleCard(Mod, modName, moduleColor, columnWidth, moduleHeight, true); // With checkbox
                    }
                }
            }
        }

        // Empty area popup
        if (ImGui::BeginPopupContextWindow("EmitterColumnEmpty", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            ImGui::TextUnformatted("Add Module");
            ImGui::Separator();
            ImGui::MenuItem("Initial Size", nullptr, false, false);
            ImGui::MenuItem("Initial Velocity", nullptr, false, false);
            ImGui::MenuItem("Color Over Life", nullptr, false, false);
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
        if (ImGui::MenuItem("New Particle Sprite Emitter"))
        {
            if (UParticleEmitter* NewEmitter = CreateDefaultSpriteEmitter())
            {
                EditingSystem->AddEmitter(NewEmitter);
                SelectedEmitterIndex = EditingSystem->GetEmitterCount() - 1;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();
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
    // Initial Color / Color Over Life
    if (UParticleModuleColor* Color = NewObject<UParticleModuleColor>())
    {
        Color->ModuleName = "Color";
        LOD0->AddModule(Color);
    }

    Emitter->AddLODLevel(LOD0);
    return Emitter;
}

void SCascadeEmittersPanel::RenderModuleCard(UParticleModule* module, const char* moduleName, const ImVec4& backgroundColor, float width, float height, bool showCheckbox)
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
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();

        // Button takes remaining width
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        if (ImGui::Button(moduleName, ImVec2(width - 30, height)))
        {
            SelectedModule = module;
        }
        ImGui::PopStyleVar();

        // Context menu on the button
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextUnformatted(moduleName);
            ImGui::Separator();
            ImGui::MenuItem("Enable/Disable", nullptr, false, false);
            ImGui::MenuItem("Delete", nullptr, false, false);
            ImGui::MenuItem("Duplicate", nullptr, false, false);
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
        }
        ImGui::PopStyleVar();

        // Context menu on the button
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextUnformatted(moduleName);
            ImGui::Separator();
            ImGui::MenuItem("Delete", nullptr, false, false);
            ImGui::MenuItem("Duplicate", nullptr, false, false);
            ImGui::EndPopup();
        }
    }

    ImGui::EndGroup();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    ImGui::PopID(); // Pop the unique ID
}

ImVec4 SCascadeEmittersPanel::GetModuleColor(const FString& moduleName)
{
    // Color scheme matching Unreal Cascade
    // Required = yellow (handled separately)
    // Spawn = red
    // Everything else = grey
    if (moduleName == "Spawn")
        return ImVec4(0.55f, 0.2f, 0.2f, 1.0f); // Red for Spawn
    else
        return ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Grey for all other modules
}

