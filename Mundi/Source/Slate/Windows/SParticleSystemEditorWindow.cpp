#include "pch.h"
#include "SParticleSystemEditorWindow.h"
#include "SlateManager.h"
#include "Source/Runtime/Engine/Viewer/ParticleSystemEditorBootstrap.h"
#include "Source/Runtime/Renderer/FViewport.h"
#include "ResourceManager.h"
#include "SCascadeEmittersPanel.h"
#include "Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Source/Runtime/Core/Object/Property.h"

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
        // BEGIN Icon Toolbar Inject
        if (!IconNew || !IconSave || !IconLoad) { LoadToolbarIcons(); }
        const ImVec2 IconSizeVec(IconSize, IconSize);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));
        ImGui::SameLine();
        if (IconNew && IconNew->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_NewBtn", (void*)IconNew->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Particle System");
        }
        else { ImGui::Button("New"); }
        ImGui::SameLine();
        if (IconLoad && IconLoad->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_LoadBtn", (void*)IconLoad->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open Particle System");
        }
        else { ImGui::Button("Open"); }
        ImGui::SameLine();
        if (IconSave && IconSave->GetShaderResourceView())
        {
            if (ImGui::ImageButton("##Cascade_SaveBtn", (void*)IconSave->GetShaderResourceView(), IconSizeVec)) { }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save Particle System");
        }
        else { ImGui::Button("Save"); }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        // END Icon Toolbar Inject
        // Hide legacy placeholder controls below
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
        ImGui::PopStyleColor(4);
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

void SParticleSystemEditorWindow::LoadToolbarIcons()
{
    if (!IconNew)
        IconNew = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_New.png");
    if (!IconSave)
        IconSave = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_Save.png");
    if (!IconLoad)
        IconLoad = UResourceManager::GetInstance().Load<UTexture>("Data/Icon/Toolbar_Load.png");
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
