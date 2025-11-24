#include "pch.h"
#include "Grid/GridActor.h"

using std::to_string;

IMPLEMENT_CLASS(AGridActor)

AGridActor::AGridActor()
{
    // Separate components for grid and axis
    GridLineComponent = NewObject<ULineComponent>();
    GridLineComponent->SetupAttachment(RootComponent);
    GridLineComponent->SetLineType("Grid");
    AddOwnedComponent(GridLineComponent);

    AxisLineComponent = NewObject<ULineComponent>();
    AxisLineComponent->SetupAttachment(RootComponent);
    AxisLineComponent->SetLineType("Axis");
    AddOwnedComponent(AxisLineComponent);
}

void AGridActor::Initialize()
{
    RegenerateGrid();
    if (EditorINI.count("GridSpacing"))
    {
        try
        {
            float temp = std::stof(EditorINI["GridSpacing"]);
            SetLineSize(temp);
        }
        catch (...)
        {
            SetLineSize(LineSize);
        }
    }
    else
    {
        SetLineSize(LineSize);
    }
}

AGridActor::~AGridActor()
{

}

namespace
{
    FVector4 GetGridLineColor(int index)
    {
        if (index % 10 == 0)
            return FVector4(1.0f, 1.0f, 1.0f, 1.0f); // 흰색
        if (index % 5 == 0)
            return FVector4(0.4f, 0.4f, 0.4f, 1.0f); // 밝은 회색
        return FVector4(0.1f, 0.1f, 0.1f, 1.0f);     // 어두운 회색
    }
}

void AGridActor::CreateGridLines(int32 InGridSize, float InCellSize, const FVector& Center)
{
    if (!GridLineComponent) return;

    const float gridTotalSize = InGridSize * InCellSize;

    for (int i = -InGridSize; i <= InGridSize; i++)
    {
        if (i == 0) continue;

        const float pos = i * InCellSize;
        const FVector4 color = GetGridLineColor(i);

        // X축 방향 라인
        GridLineComponent->AddLine(FVector(pos, -gridTotalSize, 0.0f), FVector(pos, gridTotalSize, 0.0f), color);
        // Y축 방향 라인
        GridLineComponent->AddLine(FVector(-gridTotalSize, pos, 0.0f), FVector(gridTotalSize, pos, 0.0f), color);
    }

    // 중앙 축 (X, Y)
    GridLineComponent->AddLine(FVector(-gridTotalSize, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f), FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    GridLineComponent->AddLine(FVector(0.0f, -gridTotalSize, 0.0f), FVector(0.0f, 0.0f, 0.0f), FVector4(1.0f, 1.0f, 1.0f, 1.0f));
}

void AGridActor::CreateAxisLines(float Length, const FVector& Origin)
{
    if (!AxisLineComponent) return;

    // X축 - 빨강
    AxisLineComponent->AddLine(Origin,
                          Origin + FVector(Length * CellSize, 0.0f, 0.0f),
                          FVector4(0.456f, 0.043f, 0.067f, 1.0f));

    // Y축 - 초록
    AxisLineComponent->AddLine(Origin,
                          Origin + FVector(0.0f, Length * CellSize, 0.0f),
                          FVector4(0.159f, 0.538f, 0.080f, 1.0f));

    // Z축 - 파랑
    AxisLineComponent->AddLine(Origin,
                          Origin + FVector(0.0f, 0.0f, Length * CellSize),
                          FVector4(0.054f, 0.155f, 0.527f, 1.0f));
}

void AGridActor::ClearLines()
{
    if (GridLineComponent)
    {
        GridLineComponent->ClearLines();
    }
    if (AxisLineComponent)
    {
        AxisLineComponent->ClearLines();
    }
}

void AGridActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    //LineComponent = LineComponent->Duplicate();
}

void AGridActor::RegenerateGrid()
{
    // Clear existing lines
    ClearLines();
    
    // Generate new grid and axis lines with current settings
    CreateAxisLines(AxisLength, FVector());
    CreateGridLines(GridSize, CellSize, FVector());
}

