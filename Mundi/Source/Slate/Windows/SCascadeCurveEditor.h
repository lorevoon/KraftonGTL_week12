#pragma once
#include "Object.h"
#include "ImGui/imgui.h"

class UParticleModule;

// Represents a single key point on a curve
struct FCurveKey
{
    float Time = 0.0f;
    float Value = 0.0f;
    float ArriveTangent = 0.0f;
    float LeaveTangent = 0.0f;

    // Tangent mode: 0 = Auto, 1 = User, 2 = Break, 3 = Linear, 4 = Constant
    int32 InterpMode = 0;
};

// Represents a single curve (e.g., X, Y, Z components or R, G, B, A)
struct FCurveTrack
{
    FString Name;
    ImU32 Color = IM_COL32(255, 255, 255, 255);
    TArray<FCurveKey> Keys;
    bool bVisible = true;
    bool bSelected = false;
};

// Entry in the curve list (left panel)
struct FCurveEntry
{
    FString ModuleName;
    FString PropertyName;
    TArray<FCurveTrack> Tracks;  // Multiple tracks for vector properties (X, Y, Z)
    bool bExpanded = true;
    bool bVisible = true;
    UParticleModule* OwnerModule = nullptr;
};

// The Cascade-style Curve Editor panel
// Features:
// - Left panel: List of curves organized by module/property
// - Right panel: Graph area with grid, curves, and key manipulation
// - Toolbar: Fit, zoom controls, tangent modes
class SCascadeCurveEditor
{
public:
    SCascadeCurveEditor() = default;
    ~SCascadeCurveEditor() = default;

    // Main render function
    void Render(float width, float height);

    // Curve management
    void AddCurveEntry(const FCurveEntry& Entry);
    void RemoveCurveEntry(const FString& ModuleName, const FString& PropertyName);
    void ClearAllCurves();

    // Set the module whose curves should be displayed
    void SetSelectedModule(UParticleModule* Module);
    UParticleModule* GetSelectedModule() const { return SelectedModule; }

private:
    // UI Rendering
    void RenderToolbar(float width);
    void RenderCurveList(float width, float height);
    void RenderGraphArea(float width, float height);
    void RenderGrid(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax);
    void RenderCurves(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax);
    void RenderKeyHandles(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax);

    // Coordinate conversion
    ImVec2 CurveToScreen(float time, float value, const ImVec2& canvasMin, const ImVec2& canvasMax) const;
    void ScreenToCurve(const ImVec2& screen, const ImVec2& canvasMin, const ImVec2& canvasMax, float& outTime, float& outValue) const;

    // Input handling
    void HandleGraphInput(const ImVec2& canvasMin, const ImVec2& canvasMax);
    void HandlePanning(const ImVec2& canvasMin, const ImVec2& canvasMax);
    void HandleZooming(const ImVec2& canvasMin, const ImVec2& canvasMax);
    void HandleKeySelection(const ImVec2& canvasMin, const ImVec2& canvasMax);
    void HandleKeyDragging(const ImVec2& canvasMin, const ImVec2& canvasMax);

    // Curve evaluation (for rendering smooth curves)
    float EvaluateCurve(const FCurveTrack& track, float time) const;
    float CubicInterp(float p0, float t0, float p1, float t1, float alpha) const;

private:
    // Curve data
    TArray<FCurveEntry> CurveEntries;
    UParticleModule* SelectedModule = nullptr;

    // View state
    float ViewMinTime = 0.0f;
    float ViewMaxTime = 1.0f;
    float ViewMinValue = 0.0f;
    float ViewMaxValue = 1.0f;

    // UI state
    float CurveListWidth = 200.0f;
    bool bIsPanning = false;
    bool bIsDraggingKey = false;
    bool bIsDraggingTangent = false;
    ImVec2 LastMousePos = ImVec2(0, 0);

    // Selection state
    int32 SelectedEntryIndex = -1;
    int32 SelectedTrackIndex = -1;
    int32 SelectedKeyIndex = -1;
    // -1: none, 0: arrive (left), 1: leave (right)
    int32 SelectedTangentHandle = -1;

    // Tangent editing mode
    enum class ETangentMode { Auto, User, Break, Linear, Constant };
    ETangentMode CurrentTangentMode = ETangentMode::Auto;

    // Grid settings
    bool bShowGrid = true;
    bool bSnapToGrid = false;
    float GridTimeStep = 0.1f;
    float GridValueStep = 0.1f;
};
