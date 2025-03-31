#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.h"
#include "GridManager.generated.h"

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

    // Matrice logica della griglia
    TArray<TArray<FCellData>> Grid;

    // Componente per la visualizzazione delle celle del pavimento
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* FloorMeshInstances;

    // Tre componenti per la visualizzazione degli ostacoli (con texture/materiali differenti)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* ObstacleMeshInstances3;

    // Dimensione della cella in unità world (usata per posizionare le istanze)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSize;

    // Componente per evidenziare una cella
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class UInstancedStaticMeshComponent* HighlightMeshInstances;

    // Funzione helper per ottenere la posizione world di una cella
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GetCellWorldLocation(int32 X, int32 Y) const;

    // Funzione per evidenziare una cella
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

    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> FindPathBFS(int32 startX, int32 startY, int32 endX, int32 endY) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    FString GetCellIdentifier(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    TArray<FIntPoint> FindPathAStar(int32 StartX, int32 StartY, int32 EndX, int32 EndY) const;

    FIntPoint GetNearestWalkableAdjacentCell(int32 TargetX, int32 TargetY) const;

private:
    bool IsMapConnected() const;
};
