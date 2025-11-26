#include "pch.h"
#include "SCascadeCurveEditor.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Curves/ParticleCurve.h"

void SCascadeCurveEditor::Render(float width, float height)
{
    // Main layout: Toolbar at top, then split between curve list and graph
    const float splitterWidth = 4.0f;

    // Measure toolbar height dynamically
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    float startY = startPos.y;
    RenderToolbar(width);
    float usedToolbar = ImGui::GetCursorScreenPos().y - startY;

    // Remaining height after toolbar
    float contentHeight = height - usedToolbar;
    if (contentHeight < 50.0f) contentHeight = 50.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    // Curve List (left panel)
    ImGui::BeginChild("CurveEditor_List", ImVec2(CurveListWidth, contentHeight), true, ImGuiWindowFlags_NoScrollbar);
    RenderCurveList(CurveListWidth, contentHeight);
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // Vertical splitter between list and graph
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
    ImGui::Button("##CurveEditor_VSplit", ImVec2(splitterWidth, contentHeight));
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive())
    {
        float delta = ImGui::GetIO().MouseDelta.x;
        CurveListWidth = FMath::Clamp(CurveListWidth + delta, 100.0f, width * 0.4f);
    }

    ImGui::SameLine(0, 0);

    // Graph Area (right panel)
    float graphWidth = width - CurveListWidth - splitterWidth;
    if (graphWidth < 100.0f) graphWidth = 100.0f;

    ImGui::BeginChild("CurveEditor_Graph", ImVec2(graphWidth, contentHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderGraphArea(graphWidth, contentHeight);
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void SCascadeCurveEditor::RenderToolbar(float width)
{
    // Inline toolbar (no child), dynamically sized by its contents
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

    // Fit buttons
    if (ImGui::Button("Fit All"))
    {
        // Reset view to fit all curves
        ViewMinTime = 0.0f;
        ViewMaxTime = 1.0f;
        ViewMinValue = 0.0f;
        ViewMaxValue = 1.0f;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fit view to all curves");

    ImGui::SameLine();
    if (ImGui::Button("Fit Horz"))
    {
        ViewMinTime = 0.0f;
        ViewMaxTime = 1.0f;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fit horizontal (time) axis");

    ImGui::SameLine();
    if (ImGui::Button("Fit Vert"))
    {
        ViewMinValue = 0.0f;
        ViewMaxValue = 1.0f;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fit vertical (value) axis");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Tangent mode buttons
    ImGui::TextUnformatted("Tangent:");
    ImGui::SameLine();

    bool isAuto = (CurrentTangentMode == ETangentMode::Auto);
    bool isUser = (CurrentTangentMode == ETangentMode::User);
    bool isBreak = (CurrentTangentMode == ETangentMode::Break);
    bool isLinear = (CurrentTangentMode == ETangentMode::Linear);
    bool isConst = (CurrentTangentMode == ETangentMode::Constant);

    if (isAuto) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("Auto"))
    {
        CurrentTangentMode = ETangentMode::Auto;
    }
    if (isAuto) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto tangent mode");

    ImGui::SameLine();

    if (isUser) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("User"))
    {
        CurrentTangentMode = ETangentMode::User;
    }
    if (isUser) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("User tangent mode");

    ImGui::SameLine();

    if (isBreak) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("Break"))
    {
        CurrentTangentMode = ETangentMode::Break;
    }
    if (isBreak) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Break tangents");

    ImGui::SameLine();

    if (isLinear) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("Linear"))
    {
        CurrentTangentMode = ETangentMode::Linear;
    }
    if (isLinear) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Linear interpolation");

    ImGui::SameLine();

    if (isConst) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("Const"))
    {
        CurrentTangentMode = ETangentMode::Constant;
    }
    if (isConst) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Constant (step) interpolation");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Grid toggle
    ImGui::Checkbox("Grid", &bShowGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &bSnapToGrid);

    ImGui::PopStyleVar(2);
    ImGui::Separator();
}

void SCascadeCurveEditor::RenderCurveList(float width, float height)
{
    ImGui::TextUnformatted("Curves");
    ImGui::Separator();

    // Scrollable list of curve entries
    ImGui::BeginChild("CurveList_Scroll", ImVec2(0, 0), false, ImGuiWindowFlags_None);

    if (CurveEntries.Num() == 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::TextWrapped("No curves to display.\n\nSelect a module with curve properties to see its curves here.");
        ImGui::PopStyleColor();
    }
    else
    {
        for (int32 entryIdx = 0; entryIdx < CurveEntries.Num(); ++entryIdx)
        {
            FCurveEntry& entry = CurveEntries[entryIdx];
            ImGui::PushID(entryIdx);

            // Entry header (module/property name)
            bool headerOpen = ImGui::TreeNodeEx(entry.PropertyName.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

            // Visibility toggle on the same line
            ImGui::SameLine(width - 40);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::Checkbox("##vis", &entry.bVisible))
            {
                // Toggle visibility for all tracks in this entry
                for (auto& track : entry.Tracks)
                {
                    track.bVisible = entry.bVisible;
                }
            }
            ImGui::PopStyleVar();

            if (headerOpen)
            {
                // Show individual tracks (X, Y, Z or R, G, B, A)
                for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
                {
                    FCurveTrack& track = entry.Tracks[trackIdx];
                    ImGui::PushID(trackIdx);

                    // Color indicator
                    ImVec4 trackColor = ImGui::ColorConvertU32ToFloat4(track.Color);
                    ImGui::PushStyleColor(ImGuiCol_Text, trackColor);

                    bool isSelected = (SelectedEntryIndex == entryIdx && SelectedTrackIndex == trackIdx);
                    if (isSelected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));
                    }

                    if (ImGui::Selectable(track.Name.c_str(), isSelected))
                    {
                        SelectedEntryIndex = entryIdx;
                        SelectedTrackIndex = trackIdx;
                        SelectedKeyIndex = -1;
                    }

                    if (isSelected)
                    {
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopStyleColor();

                    // Track visibility toggle
                    ImGui::SameLine(width - 40);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Checkbox("##trackvis", &track.bVisible);
                    ImGui::PopStyleVar();

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
}

void SCascadeCurveEditor::RenderGraphArea(float width, float height)
{
    ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    ImVec2 canvasMax = ImVec2(canvasMin.x + width - 16, canvasMin.y + height - 16);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 30, 255));

    // Clip to canvas area
    drawList->PushClipRect(canvasMin, canvasMax, true);

    // Render grid
    if (bShowGrid)
    {
        RenderGrid(drawList, canvasMin, canvasMax);
    }

    // Render curves
    RenderCurves(drawList, canvasMin, canvasMax);

    // Render key handles
    RenderKeyHandles(drawList, canvasMin, canvasMax);

    drawList->PopClipRect();

    // Invisible button to capture input
    ImGui::SetCursorScreenPos(canvasMin);
    ImGui::InvisibleButton("GraphCanvas", ImVec2(canvasMax.x - canvasMin.x, canvasMax.y - canvasMin.y));

    // Handle input
    HandleGraphInput(canvasMin, canvasMax);

    // Draw border
    drawList->AddRect(canvasMin, canvasMax, IM_COL32(60, 60, 60, 255));

    // Display current view range info
    ImGui::SetCursorScreenPos(ImVec2(canvasMin.x + 4, canvasMax.y - 16));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Text("Time: %.2f - %.2f | Value: %.2f - %.2f", ViewMinTime, ViewMaxTime, ViewMinValue, ViewMaxValue);
    ImGui::PopStyleColor();
}

void SCascadeCurveEditor::RenderGrid(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    const ImU32 gridColorMajor = IM_COL32(60, 60, 60, 255);
    const ImU32 gridColorMinor = IM_COL32(45, 45, 45, 255);
    const ImU32 axisColor = IM_COL32(100, 100, 100, 255);

    float canvasWidth = canvasMax.x - canvasMin.x;
    float canvasHeight = canvasMax.y - canvasMin.y;

    // Calculate grid spacing based on view range
    float timeRange = ViewMaxTime - ViewMinTime;
    float valueRange = ViewMaxValue - ViewMinValue;

    // Adaptive grid spacing
    float timeStep = GridTimeStep;
    float valueStep = GridValueStep;

    // Adjust step size based on zoom level
    while (timeRange / timeStep > 20) timeStep *= 2;
    while (timeRange / timeStep < 5) timeStep /= 2;
    while (valueRange / valueStep > 20) valueStep *= 2;
    while (valueRange / valueStep < 5) valueStep /= 2;

    // Vertical lines (time axis)
    float startTime = floor(ViewMinTime / timeStep) * timeStep;
    for (float t = startTime; t <= ViewMaxTime; t += timeStep)
    {
        ImVec2 screenPos = CurveToScreen(t, 0, canvasMin, canvasMax);
        bool isMajor = fabs(fmod(t, timeStep * 5)) < 0.001f;
        drawList->AddLine(
            ImVec2(screenPos.x, canvasMin.y),
            ImVec2(screenPos.x, canvasMax.y),
            isMajor ? gridColorMajor : gridColorMinor);

        // Time labels
        if (isMajor)
        {
            char label[16];
            sprintf_s(label, "%.1f", t);
            drawList->AddText(ImVec2(screenPos.x + 2, canvasMax.y - 14), IM_COL32(100, 100, 100, 255), label);
        }
    }

    // Horizontal lines (value axis)
    float startValue = floor(ViewMinValue / valueStep) * valueStep;
    for (float v = startValue; v <= ViewMaxValue; v += valueStep)
    {
        ImVec2 screenPos = CurveToScreen(0, v, canvasMin, canvasMax);
        bool isMajor = fabs(fmod(v, valueStep * 5)) < 0.001f;
        drawList->AddLine(
            ImVec2(canvasMin.x, screenPos.y),
            ImVec2(canvasMax.x, screenPos.y),
            isMajor ? gridColorMajor : gridColorMinor);

        // Value labels
        if (isMajor)
        {
            char label[16];
            sprintf_s(label, "%.1f", v);
            drawList->AddText(ImVec2(canvasMin.x + 2, screenPos.y - 12), IM_COL32(100, 100, 100, 255), label);
        }
    }

    // Zero axes (if visible)
    if (ViewMinTime <= 0 && ViewMaxTime >= 0)
    {
        ImVec2 screenPos = CurveToScreen(0, 0, canvasMin, canvasMax);
        drawList->AddLine(ImVec2(screenPos.x, canvasMin.y), ImVec2(screenPos.x, canvasMax.y), axisColor, 2.0f);
    }
    if (ViewMinValue <= 0 && ViewMaxValue >= 0)
    {
        ImVec2 screenPos = CurveToScreen(0, 0, canvasMin, canvasMax);
        drawList->AddLine(ImVec2(canvasMin.x, screenPos.y), ImVec2(canvasMax.x, screenPos.y), axisColor, 2.0f);
    }
}

void SCascadeCurveEditor::RenderCurves(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    const int segmentsPerKey = 16;

    for (int32 entryIdx = 0; entryIdx < CurveEntries.Num(); ++entryIdx)
    {
        const FCurveEntry& entry = CurveEntries[entryIdx];
        if (!entry.bVisible) continue;

        for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
        {
            const FCurveTrack& track = entry.Tracks[trackIdx];
            if (!track.bVisible) continue;
            if (track.Keys.Num() < 2) continue;

            // Draw curve segments
            ImVec2 prevPoint = CurveToScreen(track.Keys[0].Time, track.Keys[0].Value, canvasMin, canvasMax);

            for (int32 keyIdx = 0; keyIdx < track.Keys.Num() - 1; ++keyIdx)
            {
                const FCurveKey& key0 = track.Keys[keyIdx];
                const FCurveKey& key1 = track.Keys[keyIdx + 1];

                float timeDiff = key1.Time - key0.Time;

                for (int seg = 1; seg <= segmentsPerKey; ++seg)
                {
                    float alpha = (float)seg / segmentsPerKey;
                    float time = key0.Time + timeDiff * alpha;
                    float value = EvaluateCurve(track, time);

                    ImVec2 curPoint = CurveToScreen(time, value, canvasMin, canvasMax);

                    // Determine line thickness (thicker if selected)
                    float thickness = (SelectedEntryIndex == entryIdx && SelectedTrackIndex == trackIdx) ? 2.5f : 1.5f;

                    drawList->AddLine(prevPoint, curPoint, track.Color, thickness);
                    prevPoint = curPoint;
                }
            }
        }
    }
}

void SCascadeCurveEditor::RenderKeyHandles(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    const float keyHandleSize = 5.0f;
    const float selectedKeyHandleSize = 7.0f;

    for (int32 entryIdx = 0; entryIdx < CurveEntries.Num(); ++entryIdx)
    {
        const FCurveEntry& entry = CurveEntries[entryIdx];
        if (!entry.bVisible) continue;

        for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
        {
            const FCurveTrack& track = entry.Tracks[trackIdx];
            if (!track.bVisible) continue;

            for (int32 keyIdx = 0; keyIdx < track.Keys.Num(); ++keyIdx)
            {
                const FCurveKey& key = track.Keys[keyIdx];
                ImVec2 screenPos = CurveToScreen(key.Time, key.Value, canvasMin, canvasMax);

                bool isSelected = (SelectedEntryIndex == entryIdx &&
                                   SelectedTrackIndex == trackIdx &&
                                   SelectedKeyIndex == keyIdx);

                float size = isSelected ? selectedKeyHandleSize : keyHandleSize;
                ImU32 fillColor = isSelected ? IM_COL32(255, 255, 255, 255) : track.Color;
                ImU32 borderColor = IM_COL32(255, 255, 255, 255);

                // Draw key handle (diamond shape)
                ImVec2 points[4] = {
                    ImVec2(screenPos.x, screenPos.y - size),
                    ImVec2(screenPos.x + size, screenPos.y),
                    ImVec2(screenPos.x, screenPos.y + size),
                    ImVec2(screenPos.x - size, screenPos.y)
                };

                drawList->AddConvexPolyFilled(points, 4, fillColor);
                drawList->AddPolyline(points, 4, borderColor, ImDrawFlags_Closed, 1.0f);

                // Draw tangent handles for selected key
                if (isSelected)
                {
                    const float tangentHandleLength = 30.0f;
                    const float tangentHandleSize = 3.0f;

                    // Arrive tangent (left)
                    if (keyIdx > 0)
                    {
                        float tangentAngle = atan(key.ArriveTangent);
                        ImVec2 tangentEnd = ImVec2(
                            screenPos.x - tangentHandleLength * cos(tangentAngle),
                            screenPos.y + tangentHandleLength * sin(tangentAngle)
                        );

                        drawList->AddLine(screenPos, tangentEnd, IM_COL32(200, 200, 100, 255), 1.0f);
                        drawList->AddCircleFilled(tangentEnd, tangentHandleSize, IM_COL32(200, 200, 100, 255));
                    }

                    // Leave tangent (right)
                    if (keyIdx < track.Keys.Num() - 1)
                    {
                        float tangentAngle = atan(key.LeaveTangent);
                        ImVec2 tangentEnd = ImVec2(
                            screenPos.x + tangentHandleLength * cos(tangentAngle),
                            screenPos.y - tangentHandleLength * sin(tangentAngle)
                        );

                        drawList->AddLine(screenPos, tangentEnd, IM_COL32(200, 200, 100, 255), 1.0f);
                        drawList->AddCircleFilled(tangentEnd, tangentHandleSize, IM_COL32(200, 200, 100, 255));
                    }
                }
            }
        }
    }
}

ImVec2 SCascadeCurveEditor::CurveToScreen(float time, float value, const ImVec2& canvasMin, const ImVec2& canvasMax) const
{
    float canvasWidth = canvasMax.x - canvasMin.x;
    float canvasHeight = canvasMax.y - canvasMin.y;

    float normalizedTime = (time - ViewMinTime) / (ViewMaxTime - ViewMinTime);
    float normalizedValue = (value - ViewMinValue) / (ViewMaxValue - ViewMinValue);

    return ImVec2(
        canvasMin.x + normalizedTime * canvasWidth,
        canvasMax.y - normalizedValue * canvasHeight  // Y is inverted
    );
}

void SCascadeCurveEditor::ScreenToCurve(const ImVec2& screen, const ImVec2& canvasMin, const ImVec2& canvasMax, float& outTime, float& outValue) const
{
    float canvasWidth = canvasMax.x - canvasMin.x;
    float canvasHeight = canvasMax.y - canvasMin.y;

    float normalizedTime = (screen.x - canvasMin.x) / canvasWidth;
    float normalizedValue = (canvasMax.y - screen.y) / canvasHeight;  // Y is inverted

    outTime = ViewMinTime + normalizedTime * (ViewMaxTime - ViewMinTime);
    outValue = ViewMinValue + normalizedValue * (ViewMaxValue - ViewMinValue);
}

void SCascadeCurveEditor::HandleGraphInput(const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    HandlePanning(canvasMin, canvasMax);
    HandleZooming(canvasMin, canvasMax);
    HandleKeySelection(canvasMin, canvasMax);
    HandleKeyDragging(canvasMin, canvasMax);

    LastMousePos = ImGui::GetMousePos();
}

void SCascadeCurveEditor::HandlePanning(const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    // Middle mouse button or Alt+Left mouse button to pan
    bool isPanInput = ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
                      (ImGui::IsMouseDown(ImGuiMouseButton_Right) && ImGui::GetIO().KeyAlt);

    if (ImGui::IsItemHovered() && isPanInput)
    {
        if (!bIsPanning)
        {
            bIsPanning = true;
        }

        ImVec2 delta = ImGui::GetIO().MouseDelta;
        float canvasWidth = canvasMax.x - canvasMin.x;
        float canvasHeight = canvasMax.y - canvasMin.y;

        float timeRange = ViewMaxTime - ViewMinTime;
        float valueRange = ViewMaxValue - ViewMinValue;

        float timeDelta = -delta.x / canvasWidth * timeRange;
        float valueDelta = delta.y / canvasHeight * valueRange;

        ViewMinTime += timeDelta;
        ViewMaxTime += timeDelta;
        ViewMinValue += valueDelta;
        ViewMaxValue += valueDelta;
    }
    else
    {
        bIsPanning = false;
    }
}

void SCascadeCurveEditor::HandleZooming(const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    if (!ImGui::IsItemHovered()) return;

    float wheel = ImGui::GetIO().MouseWheel;
    if (fabs(wheel) < 0.001f) return;

    ImVec2 mousePos = ImGui::GetMousePos();

    // Get mouse position in curve space
    float mouseTime, mouseValue;
    ScreenToCurve(mousePos, canvasMin, canvasMax, mouseTime, mouseValue);

    // Zoom factor
    float zoomFactor = 1.0f - wheel * 0.1f;

    // Apply zoom centered on mouse position
    ViewMinTime = mouseTime + (ViewMinTime - mouseTime) * zoomFactor;
    ViewMaxTime = mouseTime + (ViewMaxTime - mouseTime) * zoomFactor;
    ViewMinValue = mouseValue + (ViewMinValue - mouseValue) * zoomFactor;
    ViewMaxValue = mouseValue + (ViewMaxValue - mouseValue) * zoomFactor;

    // Clamp to reasonable bounds
    float minRange = 0.001f;
    float maxRange = 1000.0f;

    if (ViewMaxTime - ViewMinTime < minRange)
    {
        float center = (ViewMinTime + ViewMaxTime) * 0.5f;
        ViewMinTime = center - minRange * 0.5f;
        ViewMaxTime = center + minRange * 0.5f;
    }
    if (ViewMaxValue - ViewMinValue < minRange)
    {
        float center = (ViewMinValue + ViewMaxValue) * 0.5f;
        ViewMinValue = center - minRange * 0.5f;
        ViewMaxValue = center + minRange * 0.5f;
    }
}

void SCascadeCurveEditor::HandleKeySelection(const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    if (!ImGui::IsItemHovered()) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    ImVec2 mousePos = ImGui::GetMousePos();
    const float hitRadius = 8.0f;

    // Ctrl + Left Click: add a new key on the active (or first visible) track
    if (ImGui::GetIO().KeyCtrl)
    {
        // Don't add if clicking near an existing key (treat as normal selection)
        for (int32 entryIdx = 0; entryIdx < CurveEntries.Num(); ++entryIdx)
        {
            const FCurveEntry& entry = CurveEntries[entryIdx];
            if (!entry.bVisible) continue;
            for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
            {
                const FCurveTrack& track = entry.Tracks[trackIdx];
                if (!track.bVisible) continue;
                for (int32 keyIdx = 0; keyIdx < track.Keys.Num(); ++keyIdx)
                {
                    ImVec2 p = CurveToScreen(track.Keys[keyIdx].Time, track.Keys[keyIdx].Value, canvasMin, canvasMax);
                    float dx = mousePos.x - p.x; float dy = mousePos.y - p.y;
                    if (dx*dx + dy*dy <= hitRadius*hitRadius)
                    {
                        // near existing key: fall through to normal selection
                        goto NORMAL_SELECTION_PATH;
                    }
                }
            }
        }

        // Choose target track: selected track or first visible
        int32 targetEntry = SelectedEntryIndex;
        int32 targetTrack = SelectedTrackIndex;
        if (targetEntry < 0 || targetEntry >= CurveEntries.Num() ||
            targetTrack < 0 || targetTrack >= CurveEntries[targetEntry].Tracks.Num() ||
            !CurveEntries[targetEntry].bVisible || !CurveEntries[targetEntry].Tracks[targetTrack].bVisible)
        {
            targetEntry = -1; targetTrack = -1;
            for (int32 e = 0; e < CurveEntries.Num(); ++e)
            {
                const FCurveEntry& entry = CurveEntries[e];
                if (!entry.bVisible) continue;
                for (int32 t = 0; t < entry.Tracks.Num(); ++t)
                {
                    if (entry.Tracks[t].bVisible)
                    {
                        targetEntry = e; targetTrack = t; break;
                    }
                }
                if (targetEntry != -1) break;
            }
        }

        if (targetEntry != -1 && targetTrack != -1)
        {
            FCurveTrack& track = CurveEntries[targetEntry].Tracks[targetTrack];
            // Convert mouse to curve space
            float newTime, newValue;
            ScreenToCurve(mousePos, canvasMin, canvasMax, newTime, newValue);
            if (bSnapToGrid)
            {
                newTime = round(newTime / GridTimeStep) * GridTimeStep;
                newValue = round(newValue / GridValueStep) * GridValueStep;
            }

            // Create key
            FCurveKey newKey; newKey.Time = newTime; newKey.Value = newValue; newKey.ArriveTangent = 0.0f; newKey.LeaveTangent = 0.0f; newKey.InterpMode = 1; // User

            // Insert sorted by time
            int32 insertIdx = 0;
            for (; insertIdx < track.Keys.Num(); ++insertIdx)
            {
                if (newTime < track.Keys[insertIdx].Time) break;
            }
            // grow and shift
            track.Keys.Add(FCurveKey());
            for (int32 i = track.Keys.Num() - 1; i > insertIdx; --i)
                track.Keys[i] = track.Keys[i - 1];
            track.Keys[insertIdx] = newKey;

            // Select the new key
            SelectedEntryIndex = targetEntry;
            SelectedTrackIndex = targetTrack;
            SelectedKeyIndex = insertIdx;
            SelectedTangentHandle = -1;
            bIsDraggingTangent = false;

            // Mark as dirty and write back immediately
            bHasPendingChanges = true;
            WriteBackChanges();
            return;
        }
    }

NORMAL_SELECTION_PATH:

    // If a key is already selected, test its tangent handles first
    if (SelectedKeyIndex >= 0 &&
        SelectedEntryIndex >= 0 && SelectedEntryIndex < CurveEntries.Num() &&
        SelectedTrackIndex >= 0 && SelectedTrackIndex < CurveEntries[SelectedEntryIndex].Tracks.Num())
    {
        const FCurveTrack& track = CurveEntries[SelectedEntryIndex].Tracks[SelectedTrackIndex];
        if (SelectedKeyIndex < track.Keys.Num())
        {
            const FCurveKey& key = track.Keys[SelectedKeyIndex];
            ImVec2 keyPos = CurveToScreen(key.Time, key.Value, canvasMin, canvasMax);
            const float tangentHandleLength = 30.0f;

            // Arrive handle (left)
            if (SelectedKeyIndex > 0)
            {
                float angle = atan(key.ArriveTangent);
                ImVec2 end = ImVec2(
                    keyPos.x - tangentHandleLength * cos(angle),
                    keyPos.y + tangentHandleLength * sin(angle)
                );
                float dx = mousePos.x - end.x; float dy = mousePos.y - end.y;
                if (dx*dx + dy*dy <= hitRadius*hitRadius)
                {
                    SelectedTangentHandle = 0; // arrive
                    bIsDraggingTangent = true;
                    return;
                }
            }
            // Leave handle (right)
            if (SelectedKeyIndex < track.Keys.Num() - 1)
            {
                float angle = atan(key.LeaveTangent);
                ImVec2 end = ImVec2(
                    keyPos.x + tangentHandleLength * cos(angle),
                    keyPos.y - tangentHandleLength * sin(angle)
                );
                float dx = mousePos.x - end.x; float dy = mousePos.y - end.y;
                if (dx*dx + dy*dy <= hitRadius*hitRadius)
                {
                    SelectedTangentHandle = 1; // leave
                    bIsDraggingTangent = true;
                    return;
                }
            }
        }
    }

    // Check if clicked on a key
    for (int32 entryIdx = 0; entryIdx < CurveEntries.Num(); ++entryIdx)
    {
        const FCurveEntry& entry = CurveEntries[entryIdx];
        if (!entry.bVisible) continue;

        for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
        {
            const FCurveTrack& track = entry.Tracks[trackIdx];
            if (!track.bVisible) continue;

            for (int32 keyIdx = 0; keyIdx < track.Keys.Num(); ++keyIdx)
            {
                const FCurveKey& key = track.Keys[keyIdx];
                ImVec2 screenPos = CurveToScreen(key.Time, key.Value, canvasMin, canvasMax);

                float dx = mousePos.x - screenPos.x;
                float dy = mousePos.y - screenPos.y;
                float dist = sqrt(dx * dx + dy * dy);

                if (dist <= hitRadius)
                {
                    SelectedEntryIndex = entryIdx;
                    SelectedTrackIndex = trackIdx;
                    SelectedKeyIndex = keyIdx;
                    SelectedTangentHandle = -1;
                    bIsDraggingTangent = false;
                    return;
                }
            }
        }
    }

    // Clicked on empty space - deselect key
    SelectedKeyIndex = -1;
    SelectedTangentHandle = -1;
    bIsDraggingTangent = false;
}

void SCascadeCurveEditor::HandleKeyDragging(const ImVec2& canvasMin, const ImVec2& canvasMax)
{
    if (SelectedKeyIndex < 0) return;
    if (SelectedEntryIndex < 0 || SelectedEntryIndex >= CurveEntries.Num()) return;
    if (SelectedTrackIndex < 0 || SelectedTrackIndex >= CurveEntries[SelectedEntryIndex].Tracks.Num()) return;

    FCurveTrack& track = CurveEntries[SelectedEntryIndex].Tracks[SelectedTrackIndex];
    if (SelectedKeyIndex >= track.Keys.Num()) return;

    bool wasEditing = bIsDraggingKey || bIsDraggingTangent;

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemActive())
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (SelectedTangentHandle != -1)
        {
            // Dragging tangent handle: update Arrive/LeaveTangent from mouse vector in curve space
            bIsDraggingTangent = true; bIsDraggingKey = false;
            float handleTime, handleValue;
            ScreenToCurve(mousePos, canvasMin, canvasMax, handleTime, handleValue);
            FCurveKey& key = track.Keys[SelectedKeyIndex];
            float dt = handleTime - key.Time;
            float dv = handleValue - key.Value;
            const float eps = 1e-5f;
            // Compute slope in curve space
            if (SelectedTangentHandle == 0) { if (dt >= -eps) dt = -eps; }
            else { if (dt <= eps) dt = eps; }
            float slope = dv / dt;

            // Switch to User mode on manual edit (unless explicitly Break)
            if (key.InterpMode == 0 /*Auto*/ || key.InterpMode == 3 /*Linear*/ || key.InterpMode == 4 /*Constant*/)
                key.InterpMode = 1; // User

            // If not Break (2), unify tangents (UE-style locked tangents)
            if (key.InterpMode != 2 /*Break*/)
            {
                key.ArriveTangent = slope;
                key.LeaveTangent = slope;
            }
            else
            {
                // Break mode: edit only the selected side
                if (SelectedTangentHandle == 0) key.ArriveTangent = slope; else key.LeaveTangent = slope;
            }

            // Mark as dirty for live preview
            bHasPendingChanges = true;
        }
        else
        {
            // Dragging key: move time/value
            bIsDraggingKey = true; bIsDraggingTangent = false;
            float newTime, newValue;
            ScreenToCurve(mousePos, canvasMin, canvasMax, newTime, newValue);

            if (bSnapToGrid)
            {
                newTime = round(newTime / GridTimeStep) * GridTimeStep;
                newValue = round(newValue / GridValueStep) * GridValueStep;
            }

            track.Keys[SelectedKeyIndex].Time = newTime;
            track.Keys[SelectedKeyIndex].Value = newValue;

            // Mark as dirty for live preview
            bHasPendingChanges = true;
        }

        // Write back changes immediately for live preview
        if (bHasPendingChanges)
        {
            WriteBackChanges();
            bHasPendingChanges = true; // Keep dirty until mouse release
        }
    }
    else
    {
        // Mouse released - finalize changes
        if (wasEditing && bHasPendingChanges)
        {
            WriteBackChanges();
        }
        bIsDraggingKey = false;
        bIsDraggingTangent = false;
    }
}

float SCascadeCurveEditor::EvaluateCurve(const FCurveTrack& track, float time) const
{
    if (track.Keys.Num() == 0) return 0.0f;
    if (track.Keys.Num() == 1) return track.Keys[0].Value;

    // Find the two keys surrounding this time
    int32 keyIdx = 0;
    for (int32 i = 0; i < track.Keys.Num() - 1; ++i)
    {
        if (time >= track.Keys[i].Time && time <= track.Keys[i + 1].Time)
        {
            keyIdx = i;
            break;
        }
        keyIdx = i;
    }

    // Clamp to first/last key if outside range
    if (time <= track.Keys[0].Time) return track.Keys[0].Value;
    if (time >= track.Keys[track.Keys.Num() - 1].Time) return track.Keys[track.Keys.Num() - 1].Value;

    const FCurveKey& key0 = track.Keys[keyIdx];
    const FCurveKey& key1 = track.Keys[keyIdx + 1];

    float timeDiff = key1.Time - key0.Time;
    if (timeDiff < 0.0001f) return key0.Value;

    float alpha = (time - key0.Time) / timeDiff;

    // Hermite interpolation
    return CubicInterp(key0.Value, key0.LeaveTangent * timeDiff,
                       key1.Value, key1.ArriveTangent * timeDiff, alpha);
}

float SCascadeCurveEditor::CubicInterp(float p0, float t0, float p1, float t1, float alpha) const
{
    // Hermite basis functions
    float alpha2 = alpha * alpha;
    float alpha3 = alpha2 * alpha;

    float h00 = 2 * alpha3 - 3 * alpha2 + 1;
    float h10 = alpha3 - 2 * alpha2 + alpha;
    float h01 = -2 * alpha3 + 3 * alpha2;
    float h11 = alpha3 - alpha2;

    return h00 * p0 + h10 * t0 + h01 * p1 + h11 * t1;
}

void SCascadeCurveEditor::AddCurveEntry(const FCurveEntry& Entry)
{
    CurveEntries.Add(Entry);
}

void SCascadeCurveEditor::RemoveCurveEntry(const FString& ModuleName, const FString& PropertyName)
{
    for (int32 i = CurveEntries.Num() - 1; i >= 0; --i)
    {
        if (CurveEntries[i].ModuleName == ModuleName && CurveEntries[i].PropertyName == PropertyName)
        {
            CurveEntries.RemoveAt(i);
            break;
        }
    }
}

void SCascadeCurveEditor::ClearAllCurves()
{
    CurveEntries.Empty();
    SelectedEntryIndex = -1;
    SelectedTrackIndex = -1;
    SelectedKeyIndex = -1;
}

void SCascadeCurveEditor::SetSelectedModule(UParticleModule* Module)
{
    if (SelectedModule == Module) return;

    // Write back any pending changes before switching
    if (bHasPendingChanges)
    {
        WriteBackChanges();
    }

    SelectedModule = Module;
    ClearAllCurves();
    bHasPendingChanges = false;

    if (Module)
    {
        LoadCurvesFromModule();
    }
}

void SCascadeCurveEditor::LoadCurvesFromModule()
{
    CurveEntries.Empty();
    SelectedEntryIndex = -1;
    SelectedTrackIndex = -1;
    SelectedKeyIndex = -1;

    if (!SelectedModule) return;

    // Get curve properties from the module
    TArray<FCurvePropertyInfo> CurveProperties;
    SelectedModule->GetCurveProperties(CurveProperties);

    for (const FCurvePropertyInfo& PropInfo : CurveProperties)
    {
        if (!PropInfo.CurvePtr) continue;

        FCurveEntry entry;
        entry.ModuleName = SelectedModule->ModuleName;
        entry.PropertyName = PropInfo.PropertyName;
        entry.bExpanded = true;
        entry.bVisible = true;
        entry.OwnerModule = SelectedModule;
        entry.SourceCurve = PropInfo.CurvePtr;
        entry.bUseCurvePtr = PropInfo.bUseCurvePtr;

        // Create tracks for each channel
        for (int32 channelIdx = 0; channelIdx < PropInfo.NumChannels; ++channelIdx)
        {
            FCurveTrack track;
            track.Name = (channelIdx < PropInfo.ChannelNames.Num()) ? PropInfo.ChannelNames[channelIdx] : FString("Channel %d", channelIdx);
            track.Color = (channelIdx < PropInfo.ChannelColors.Num()) ? PropInfo.ChannelColors[channelIdx] : IM_COL32(255, 255, 255, 255);
            track.bVisible = true;
            track.bSelected = false;

            // Read keys from the curve
            FParticleCurve* Curve = PropInfo.CurvePtr;
            if (channelIdx < Curve->Channels.Num())
            {
                const FParticleCurveChannel& Channel = Curve->Channels[channelIdx];
                for (const FParticleCurveKey& pk : Channel.Keys)
                {
                    FCurveKey key;
                    key.Time = pk.Time;
                    key.Value = pk.Value;
                    key.ArriveTangent = pk.ArriveTangent;
                    key.LeaveTangent = pk.LeaveTangent;
                    key.InterpMode = pk.InterpMode;
                    track.Keys.Add(key);
                }
            }

            // If no keys exist, create a default linear curve (0 to 1)
            if (track.Keys.Num() == 0)
            {
                FCurveKey key0, key1;
                key0.Time = 0.0f; key0.Value = 1.0f; key0.InterpMode = 3; // Linear
                key1.Time = 1.0f; key1.Value = 1.0f; key1.InterpMode = 3; // Linear
                track.Keys.Add(key0);
                track.Keys.Add(key1);
            }

            entry.Tracks.Add(track);
        }

        CurveEntries.Add(entry);
    }
}

void SCascadeCurveEditor::WriteBackChanges()
{
    if (!bHasPendingChanges) return;

    // Write each entry back to its source curve
    for (const FCurveEntry& entry : CurveEntries)
    {
        if (!entry.SourceCurve) continue;

        FParticleCurve* Curve = entry.SourceCurve;

        // Enable curve mode on the module
        if (entry.bUseCurvePtr)
        {
            *entry.bUseCurvePtr = true;
        }

        // Ensure curve has enough channels
        while (Curve->Channels.Num() < entry.Tracks.Num())
        {
            Curve->Channels.Add(FParticleCurveChannel());
        }

        // Write each track back to its channel
        for (int32 trackIdx = 0; trackIdx < entry.Tracks.Num(); ++trackIdx)
        {
            const FCurveTrack& track = entry.Tracks[trackIdx];
            FParticleCurveChannel& Channel = Curve->Channels[trackIdx];

            Channel.Keys.Empty();
            for (const FCurveKey& key : track.Keys)
            {
                FParticleCurveKey pk;
                pk.Time = key.Time;
                pk.Value = key.Value;
                pk.ArriveTangent = key.ArriveTangent;
                pk.LeaveTangent = key.LeaveTangent;
                pk.InterpMode = key.InterpMode;
                Channel.Keys.Add(pk);
            }
        }
    }

    bHasPendingChanges = false;
}
