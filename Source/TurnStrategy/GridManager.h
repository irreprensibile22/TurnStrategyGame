#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.h"
#include "GridManager.generated.h"

// State of a single cell. A cell is walkable only if it's neither an obstacle nor occupied.
USTRUCT(BlueprintType)
struct FCellData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    int32 Y = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    bool bIsObstacle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    bool bIsOccupied = false;
};

// Owns the board: generates it, draws it, and answers pathfinding/distance queries.
UCLASS()
class AGridManager : public AActor
{
    GENERATED_BODY()

public:
    AGridManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float ObstaclesPercentage;

    // Logical grid, indexed as Grid[X][Y]
    TArray<TArray<FCellData>> Grid;

    // Instanced mesh for the floor cells
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* FloorMeshInstances;

    // Three components for obstacles, one per texture (mountain + two kinds of trees)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances3;

    // Cell size in world units (used to place the instances)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSize;

    // Shared overlay for every highlight (hover, selection, movement range).
    // Only one highlight set is on screen at a time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* HighlightMeshInstances;

    // Helper: world-space center of a cell
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GetCellWorldLocation(int32 X, int32 Y) const;

    // Highlights a single cell (clears any previous highlight)
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightCell(int32 X, int32 Y);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetHighlightColor(const FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightMovementRange(int32 StartX, int32 StartY, int32 MovementRange, const FLinearColor& RangeColor);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearMovementRange();

    UFUNCTION(BlueprintCallable, Category = "Highlight")
    void HighlightOpponentMovementRange(const TArray<ABaseUnit*>& OpponentUnits, const FLinearColor& RangeColor);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Background")
    UStaticMeshComponent* BackgroundMesh;

protected:
    virtual void BeginPlay() override;
    void GenerateGrid();

public:
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FCellData GetCell(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool IsCellWalkable(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetCellOccupied(int32 X, int32 Y, bool bOccupied);

    // Both pathfinders return the full path including the start cell, or an empty
    // array if the destination can't be reached. BFS is used for the player, A* for the AI.
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> FindPathBFS(int32 startX, int32 startY, int32 endX, int32 endY) const;

    // Chess-like name for the move log, e.g. (0,0) -> "A1"
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FString GetCellIdentifier(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    TArray<FIntPoint> FindPathAStar(int32 StartX, int32 StartY, int32 EndX, int32 EndY) const;

    // First free cell next to the target, or (-1,-1) if it's boxed in.
    FIntPoint GetNearestWalkableAdjacentCell(int32 TargetX, int32 TargetY) const;

    // Cells reachable (shortest path) at distance 1..MaxRange from the start cell.
    TArray<FIntPoint> GetReachableCells(int32 StartX, int32 StartY, int32 MaxRange) const;

    // Shortest-path distance from one cell to every other cell, with a single BFS.
    // Unreachable cells are -1. The start cell is 0.
    TArray<TArray<int32>> GetDistanceMap(int32 StartX, int32 StartY) const;

private:
    // True if every non-obstacle cell can reach every other one (no isolated pockets).
    bool IsMapConnected() const;
};
