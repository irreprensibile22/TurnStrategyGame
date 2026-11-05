#include "GridManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Containers/Queue.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;

    GridSize = 25;
    ObstaclesPercentage = 10.f;
    CellSize = 100.f;

    FloorMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorMeshInstances"));
    RootComponent = FloorMeshInstances;

    ObstacleMeshInstances1 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ObstacleMeshInstances1"));
    ObstacleMeshInstances1->SetupAttachment(RootComponent);

    ObstacleMeshInstances2 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ObstacleMeshInstances2"));
    ObstacleMeshInstances2->SetupAttachment(RootComponent);

    ObstacleMeshInstances3 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ObstacleMeshInstances3"));
    ObstacleMeshInstances3->SetupAttachment(RootComponent);

    HighlightMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HighlightMeshInstances"));
    HighlightMeshInstances->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HighlightMatAsset(TEXT("Material'/Game/texture/M_HighlightBase.M_HighlightBase'"));
    if (HighlightMatAsset.Succeeded())
    {
        UMaterialInstanceDynamic* DynamicHighlightMat = UMaterialInstanceDynamic::Create(HighlightMatAsset.Object, this);
        if (DynamicHighlightMat)
        {
            DynamicHighlightMat->SetVectorParameterValue(FName("HighlightColor"), FLinearColor::White);
            HighlightMeshInstances->SetMaterial(0, DynamicHighlightMat);
        }
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> HighlightMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (HighlightMeshAsset.Succeeded())
    {
        HighlightMeshInstances->SetStaticMesh(HighlightMeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (FloorMeshAsset.Succeeded())
    {
        FloorMeshInstances->SetStaticMesh(FloorMeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ObstacleMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (ObstacleMeshAsset.Succeeded())
    {
        ObstacleMeshInstances1->SetStaticMesh(ObstacleMeshAsset.Object);
        ObstacleMeshInstances2->SetStaticMesh(ObstacleMeshAsset.Object);
        ObstacleMeshInstances3->SetStaticMesh(ObstacleMeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMatAsset(TEXT("Material'/Game/texture/M_WhiteGrid.M_WhiteGrid'"));
    if (FloorMatAsset.Succeeded())
    {
        FloorMeshInstances->SetMaterial(0, FloorMatAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObstacleMatAsset1(TEXT("Material'/Game/texture/Mountain1.Mountain1'"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObstacleMatAsset2(TEXT("Material'/Game/texture/Tree11.Tree11'"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObstacleMatAsset3(TEXT("Material'/Game/texture/Tree22.Tree22'"));
    if (ObstacleMatAsset1.Succeeded())
    {
        ObstacleMeshInstances1->SetMaterial(0, ObstacleMatAsset1.Object);
    }
    if (ObstacleMatAsset2.Succeeded())
    {
        ObstacleMeshInstances2->SetMaterial(0, ObstacleMatAsset2.Object);
    }
    if (ObstacleMatAsset3.Succeeded())
    {
        ObstacleMeshInstances3->SetMaterial(0, ObstacleMatAsset3.Object);
    }

    HighlightMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);



    BackgroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundMesh"));
    BackgroundMesh->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BackgroundMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (BackgroundMeshAsset.Succeeded())
    {
        BackgroundMesh->SetStaticMesh(BackgroundMeshAsset.Object);

        BackgroundMesh->SetRelativeLocation(FVector(0.f, 0.f, 290.f));

        float GridWorldSize = GridSize * CellSize;

        float BackgroundSizeFactor = 3.f;

        FVector NewScale = FVector((GridWorldSize * BackgroundSizeFactor) / 100.f, (GridWorldSize * BackgroundSizeFactor) / 100.f, 1.f);
        BackgroundMesh->SetWorldScale3D(NewScale);

        static ConstructorHelpers::FObjectFinder<UMaterialInterface> BackgroundMatAsset(TEXT("Material'/Game/texture/Sfondogiocoo.Sfondogiocoo'"));
        if (BackgroundMatAsset.Succeeded())
        {
            BackgroundMesh->SetMaterial(0, BackgroundMatAsset.Object);
        }
    }
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateGrid();
}

void AGridManager::GenerateGrid()
{
    Grid.SetNum(GridSize);
    for (int32 x = 0; x < GridSize; x++)
    {
        Grid[x].SetNum(GridSize);
        for (int32 y = 0; y < GridSize; y++)
        {
            FCellData& Cell = Grid[x][y];
            Cell.X = x;
            Cell.Y = y;
            Cell.bIsObstacle = false;
            Cell.bIsOccupied = false;
        }
    }

    int32 TotalCells = GridSize * GridSize;
    int32 ObstaclesCount = FMath::RoundToInt(TotalCells * (ObstaclesPercentage / 100.f));

    bool bConnected = false;
    while (!bConnected)
    {
        for (int32 x = 0; x < GridSize; x++)
        {
            for (int32 y = 0; y < GridSize; y++)
            {
                Grid[x][y].bIsObstacle = false;
            }
        }
        int32 Placed = 0;
        while (Placed < ObstaclesCount)
        {
            int32 RandomX = FMath::RandRange(0, GridSize - 1);
            int32 RandomY = FMath::RandRange(0, GridSize - 1);
            FCellData& Cell = Grid[RandomX][RandomY];
            if (!Cell.bIsObstacle)
            {
                Cell.bIsObstacle = true;
                Placed++;
            }
        }
        bConnected = IsMapConnected();
    }

    FloorMeshInstances->ClearInstances();
    ObstacleMeshInstances1->ClearInstances();
    ObstacleMeshInstances2->ClearInstances();
    ObstacleMeshInstances3->ClearInstances();

    FVector GridOffset = FVector(GridSize * CellSize * 0.5f, GridSize * CellSize * 0.5f, 0.f);

    float CellGap = 5.f; 
    float ScaleFactor = (CellSize - CellGap) / CellSize; 

    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            FVector CellLocation = FVector(x * CellSize, y * CellSize, 300.0f) - GridOffset;
            FTransform InstanceTransform;
            InstanceTransform.SetLocation(CellLocation);

            InstanceTransform.SetScale3D(FVector(ScaleFactor, ScaleFactor, 1.f));

            if (Grid[x][y].bIsObstacle)
            {
                int32 RandomType = FMath::RandRange(0, 2);
                if (RandomType == 0)
                {
                    ObstacleMeshInstances1->AddInstance(InstanceTransform);
                }
                else if (RandomType == 1)
                {
                    ObstacleMeshInstances2->AddInstance(InstanceTransform);
                }
                else
                {
                    ObstacleMeshInstances3->AddInstance(InstanceTransform);
                }
            }
            else
            {
                FloorMeshInstances->AddInstance(InstanceTransform);
            }
        }
    }
}


FCellData AGridManager::GetCell(int32 X, int32 Y) const
{
    if (X >= 0 && X < GridSize && Y >= 0 && Y < GridSize)
    {
        return Grid[X][Y];
    }
    return FCellData();
}

bool AGridManager::IsCellWalkable(int32 X, int32 Y) const
{
    if (X < 0 || X >= GridSize || Y < 0 || Y >= GridSize)
    {
        return false;
    }
    const FCellData& Cell = Grid[X][Y];
    return (!Cell.bIsObstacle && !Cell.bIsOccupied);
}

void AGridManager::SetCellOccupied(int32 X, int32 Y, bool bOccupied)
{
    if (X < 0 || X >= GridSize || Y < 0 || Y >= GridSize)
    {
        return;
    }
    Grid[X][Y].bIsOccupied = bOccupied;
}

TArray<FIntPoint> AGridManager::FindPathBFS(int32 startX, int32 startY, int32 endX, int32 endY) const
{
    TArray<FIntPoint> Path;
    if (!IsCellWalkable(endX, endY))
    {
        if (!(startX == endX && startY == endY))
        {
            return Path;
        }
    }

    TQueue<FIntPoint> Open;
    TMap<FIntPoint, FIntPoint> CameFrom;
    FIntPoint Start(startX, startY);
    Open.Enqueue(Start);
    CameFrom.Add(Start, FIntPoint(-1, -1));
    bool bFoundPath = false;

    while (!Open.IsEmpty())
    {
        FIntPoint Current;
        Open.Dequeue(Current);
        if (Current == FIntPoint(endX, endY))
        {
            bFoundPath = true;
            break;
        }
        TArray<FIntPoint> Directions = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint Neighbor(Current.X + Dir.X, Current.Y + Dir.Y);
            if (Neighbor.X >= 0 && Neighbor.X < GridSize && Neighbor.Y >= 0 && Neighbor.Y < GridSize)
            {
                if (IsCellWalkable(Neighbor.X, Neighbor.Y) && !CameFrom.Contains(Neighbor))
                {
                    Open.Enqueue(Neighbor);
                    CameFrom.Add(Neighbor, Current);
                }
            }
        }
    }

    if (!bFoundPath)
    {
        return Path;
    }

    FIntPoint CurrentPoint(endX, endY);
    while (CurrentPoint != Start)
    {
        Path.Insert(CurrentPoint, 0);
        CurrentPoint = CameFrom[CurrentPoint];
    }
    Path.Insert(Start, 0);


    return Path;
}



TArray<FIntPoint> AGridManager::FindPathAStar(int32 StartX, int32 StartY, int32 EndX, int32 EndY) const
{
    TArray<FIntPoint> Path;

    if (!IsCellWalkable(EndX, EndY) && !(StartX == EndX && StartY == EndY))
    {
        return Path;
    }

    struct FAStarNode
    {
        FIntPoint Position;
        int32 GCost;
        int32 HCost;

        int32 GetFCost() const
        {
            return GCost + HCost;
        }
    };

    // Predicato per il min-heap: ordina per F crescente, a parita' di F per H crescente.
    struct FAStarNodeLess
    {
        bool operator()(const FAStarNode& A, const FAStarNode& B) const
        {
            const int32 FA = A.GetFCost();
            const int32 FB = B.GetFCost();
            if (FA != FB)
            {
                return FA < FB;
            }
            return A.HCost < B.HCost;
        }
    };

    auto Heuristic = [&](int32 X1, int32 Y1, int32 X2, int32 Y2)
        {
            return FMath::Abs(X1 - X2) + FMath::Abs(Y1 - Y2);
        };

    FIntPoint Start(StartX, StartY);
    FIntPoint Goal(EndX, EndY);

    // OpenList gestita come min-heap binario (HeapPush/HeapPop): estrazione del minimo in O(log V).
    // Si usa la "lazy deletion": invece di aggiornare un nodo gia' in heap, se ne inserisce uno
    // nuovo con costo migliore; i duplicati obsoleti vengono scartati al momento del pop.
    TArray<FAStarNode> OpenList;
    TSet<FIntPoint> ClosedSet;

    TMap<FIntPoint, int32> GCostMap;
    TMap<FIntPoint, FIntPoint> ParentMap;

    FAStarNode StartNode;
    StartNode.Position = Start;
    StartNode.GCost = 0;
    StartNode.HCost = Heuristic(StartX, StartY, EndX, EndY);

    OpenList.HeapPush(StartNode, FAStarNodeLess());
    GCostMap.Add(Start, 0);
    ParentMap.Add(Start, FIntPoint(-1, -1));

    bool bFoundGoal = false;

    while (OpenList.Num() > 0)
    {
        FAStarNode CurrentNode;
        OpenList.HeapPop(CurrentNode, FAStarNodeLess());

        // Nodo obsoleto (lazy deletion): gia' espanso, oppure superato da un costo migliore.
        if (ClosedSet.Contains(CurrentNode.Position))
        {
            continue;
        }
        const int32* BestG = GCostMap.Find(CurrentNode.Position);
        if (BestG && CurrentNode.GCost > *BestG)
        {
            continue;
        }

        if (CurrentNode.Position == Goal)
        {
            bFoundGoal = true;
            break;
        }

        ClosedSet.Add(CurrentNode.Position);

        const TArray<FIntPoint> Directions = { FIntPoint(1,0), FIntPoint(-1,0),
                                               FIntPoint(0,1), FIntPoint(0,-1) };

        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint Neighbor(CurrentNode.Position.X + Dir.X,
                CurrentNode.Position.Y + Dir.Y);

            if (Neighbor.X < 0 || Neighbor.X >= GridSize ||
                Neighbor.Y < 0 || Neighbor.Y >= GridSize)
            {
                continue;
            }
            if (!IsCellWalkable(Neighbor.X, Neighbor.Y) && Neighbor != Goal)
            {
                continue;
            }
            if (ClosedSet.Contains(Neighbor))
            {
                continue;
            }

            int32 TentativeGCost = CurrentNode.GCost + 1;

            const int32* NeighborG = GCostMap.Find(Neighbor);
            if (!NeighborG || TentativeGCost < *NeighborG)
            {
                GCostMap.Add(Neighbor, TentativeGCost);
                ParentMap.Add(Neighbor, CurrentNode.Position);

                FAStarNode NeighborNode;
                NeighborNode.Position = Neighbor;
                NeighborNode.GCost = TentativeGCost;
                NeighborNode.HCost = Heuristic(Neighbor.X, Neighbor.Y, EndX, EndY);

                OpenList.HeapPush(NeighborNode, FAStarNodeLess());
            }
        }
    }

    if (!bFoundGoal)
    {
        return Path;
    }

    FIntPoint CurrentPoint = EndX == StartX && EndY == StartY ? Start : FIntPoint(EndX, EndY);
    while (!(CurrentPoint.X == -1 && CurrentPoint.Y == -1))
    {
        Path.Insert(CurrentPoint, 0);

        bool bHasParent = false;

        if (ParentMap.Contains(CurrentPoint))
        {
            FIntPoint TheParent = ParentMap[CurrentPoint];
            if (TheParent.X != -1 || TheParent.Y != -1)
            {
                bHasParent = true;
                CurrentPoint = TheParent;
            }
        }

        if (!bHasParent)
        {
            break;
        }
    }
    return Path;
}





FString AGridManager::GetCellIdentifier(int32 X, int32 Y) const
{
    if (X < 0 || X >= GridSize || Y < 0 || Y >= GridSize)
    {
        return FString("Invalid");
    }
    TCHAR Letter = 'A' + X;
    int32 Number = Y + 1;
    return FString::Printf(TEXT("%c%d"), Letter, Number);
}

bool AGridManager::IsMapConnected() const
{
    int32 TotalWalkable = 0;
    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            if (!Grid[x][y].bIsObstacle)
            {
                TotalWalkable++;
            }
        }
    }
    FIntPoint Start(-1, -1);
    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            if (!Grid[x][y].bIsObstacle)
            {
                Start = FIntPoint(x, y);
                break;
            }
        }
        if (Start.X != -1)
        {
            break;
        }
    }
    if (Start.X == -1)
    {
        return false;
    }
    TQueue<FIntPoint> Queue;
    TSet<FIntPoint> Visited;
    Queue.Enqueue(Start);
    Visited.Add(Start);
    while (!Queue.IsEmpty())
    {
        FIntPoint Current;
        Queue.Dequeue(Current);
        TArray<FIntPoint> Directions = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };
        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint Neighbor(Current.X + Dir.X, Current.Y + Dir.Y);
            if (Neighbor.X >= 0 && Neighbor.X < GridSize && Neighbor.Y >= 0 && Neighbor.Y < GridSize)
            {
                if (!Grid[Neighbor.X][Neighbor.Y].bIsObstacle && !Visited.Contains(Neighbor))
                {
                    Queue.Enqueue(Neighbor);
                    Visited.Add(Neighbor);
                }
            }
        }
    }
    return (Visited.Num() == TotalWalkable);
}



FVector AGridManager::GetCellWorldLocation(int32 X, int32 Y) const
{
    FVector GridOffset = FVector(GridSize * CellSize * 0.5f, GridSize * CellSize * 0.5f, 0.f);
    return FVector(X * CellSize, Y * CellSize, 300.f) - GridOffset;
}

void AGridManager::HighlightCell(int32 X, int32 Y)
{
    if (HighlightMeshInstances)
    {
        HighlightMeshInstances->ClearInstances();
        FVector Location = GetCellWorldLocation(X, Y);
        Location.Z += 10.0f;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetScale3D(FVector(1.1f, 1.1f, 1.f));
        HighlightMeshInstances->AddInstance(Transform);
    }
}



void AGridManager::SetHighlightColor(const FLinearColor& NewColor)
{
    if (HighlightMeshInstances)
    {
        UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(HighlightMeshInstances->GetMaterial(0));
        if (DynMat)
        {
            DynMat->SetVectorParameterValue(FName("HighlightColor"), NewColor);
        }
    }
}



void AGridManager::HighlightMovementRange(int32 StartX, int32 StartY, int32 MovementRange, const FLinearColor& RangeColor)
{
    if (!HighlightMeshInstances)
        return;


    UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(HighlightMeshInstances->GetMaterial(0));
    if (DynMat)
    {
        DynMat->SetVectorParameterValue(FName("HighlightColor"), RangeColor);
    }

    HighlightMeshInstances->ClearInstances();

    float CellGap = 5.f;
    float ScaleFactor = (CellSize - CellGap) / CellSize;

    // Una sola BFS dalla cella di partenza: O(N) invece di una BFS per ogni cella (O(N^2)).
    TArray<TArray<int32>> Dist = GetDistanceMap(StartX, StartY);

    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            int32 Distance = Dist[x][y];
            if (Distance >= 1 && Distance <= MovementRange)
            {
                FVector CellLocation = GetCellWorldLocation(x, y);
                CellLocation.Z += 5.f;

                FTransform Transform;
                Transform.SetLocation(CellLocation);
                Transform.SetScale3D(FVector(ScaleFactor, ScaleFactor, 1.0f));

                HighlightMeshInstances->AddInstance(Transform);
            }
        }
    }
}







void AGridManager::ClearMovementRange()
{
    if (HighlightMeshInstances)
    {
        HighlightMeshInstances->ClearInstances();
    }
}






TArray<TArray<int32>> AGridManager::GetDistanceMap(int32 StartX, int32 StartY) const
{
    // Distanza -1 = non raggiungibile. Una sola BFS (flood-fill) copre tutta la griglia: O(N).
    TArray<TArray<int32>> Dist;
    Dist.SetNum(GridSize);
    for (int32 x = 0; x < GridSize; x++)
    {
        Dist[x].Init(-1, GridSize);
    }

    if (StartX < 0 || StartX >= GridSize || StartY < 0 || StartY >= GridSize)
    {
        return Dist;
    }

    TQueue<FIntPoint> Queue;
    Dist[StartX][StartY] = 0;
    Queue.Enqueue(FIntPoint(StartX, StartY));

    const TArray<FIntPoint> Directions = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };

    while (!Queue.IsEmpty())
    {
        FIntPoint Current;
        Queue.Dequeue(Current);
        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint Neighbor(Current.X + Dir.X, Current.Y + Dir.Y);
            if (Neighbor.X < 0 || Neighbor.X >= GridSize || Neighbor.Y < 0 || Neighbor.Y >= GridSize)
            {
                continue;
            }
            if (Dist[Neighbor.X][Neighbor.Y] != -1)
            {
                continue; // gia' visitata
            }
            if (!IsCellWalkable(Neighbor.X, Neighbor.Y))
            {
                continue; // ostacolo o cella occupata
            }
            Dist[Neighbor.X][Neighbor.Y] = Dist[Current.X][Current.Y] + 1;
            Queue.Enqueue(Neighbor);
        }
    }

    return Dist;
}

TArray<FIntPoint> AGridManager::GetReachableCells(int32 StartX, int32 StartY, int32 MaxRange) const
{
    TArray<FIntPoint> Reachable;
    TArray<TArray<int32>> Dist = GetDistanceMap(StartX, StartY);

    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            int32 D = Dist[x][y];
            if (D >= 1 && D <= MaxRange)
            {
                Reachable.Add(FIntPoint(x, y));
            }
        }
    }
    return Reachable;
}

FIntPoint AGridManager::GetNearestWalkableAdjacentCell(int32 TargetX, int32 TargetY) const
{
    TArray<FIntPoint> Directions = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };
    for (const FIntPoint& Dir : Directions)
    {
        int32 AdjX = TargetX + Dir.X;
        int32 AdjY = TargetY + Dir.Y;
        if (IsCellWalkable(AdjX, AdjY))
        {
            return FIntPoint(AdjX, AdjY);
        }
    }
    return FIntPoint(-1, -1);
}







void AGridManager::HighlightOpponentMovementRange(const TArray<ABaseUnit*>& OpponentUnits, const FLinearColor& RangeColor)
{
    if (!HighlightMeshInstances)
        return;

    UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(HighlightMeshInstances->GetMaterial(0));
    if (DynMat)
    {
        DynMat->SetVectorParameterValue(FName("HighlightColor"), RangeColor);
    }

    HighlightMeshInstances->ClearInstances();

    float CellGap = 5.f;
    float ScaleFactor = (CellSize - CellGap) / CellSize;

    TSet<FIntPoint> CellsToHighlight;

    for (ABaseUnit* Unit : OpponentUnits)
    {
        if (!Unit || !Unit->IsAlive() || Unit->bHasMoved)
            continue;

        // Una sola BFS per unita' (O(N)) invece di una BFS per ogni cella (O(N^2)).
        TArray<TArray<int32>> Dist = GetDistanceMap(Unit->CurrentX, Unit->CurrentY);
        for (int32 x = 0; x < GridSize; x++)
        {
            for (int32 y = 0; y < GridSize; y++)
            {
                int32 Distance = Dist[x][y];
                if (Distance >= 1 && Distance <= Unit->MaxMovement)
                {
                    CellsToHighlight.Add(FIntPoint(x, y));
                }
            }
        }
    }

    for (const FIntPoint& Cell : CellsToHighlight)
    {
        FVector CellLocation = GetCellWorldLocation(Cell.X, Cell.Y);
        CellLocation.Z += 5.f; 
        FTransform Transform;
        Transform.SetLocation(CellLocation);
        Transform.SetScale3D(FVector(ScaleFactor, ScaleFactor, 1.0f));
        HighlightMeshInstances->AddInstance(Transform);
    }
}
