#include "GameManager.h"
#include "BaseUnit.h"
#include "Sniper.h"
#include "Brawler.h"
#include "GridManager.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include <Kismet/GameplayStatics.h>
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MyUI.h"
#include "Kismet/KismetSystemLibrary.h"

AGameManager::AGameManager()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentPhase = EGamePhase::EPlacement;
    bPlayerWinsCoinToss = false;
    bIsPlayerTurn = false;
    UnitsToPlacePerSide = 2;
    // No pawn: the player only interacts through the mouse and a fixed camera.
    DefaultPawnClass = nullptr;
    PlayerUnitsPlaced = 0;
    OpponentUnitsPlaced = 0;
    SelectedUnit = nullptr;

    static ConstructorHelpers::FClassFinder<UMyUI> UIBPClass(TEXT("/Game/HUD_Gioco/MyUI_Blueprint"));
    if (UIBPClass.Class != nullptr)
    {
        UIGameClass = UIBPClass.Class;
    }

    bAiTurnPending = false;
}

void AGameManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (Controller)
    {
        UI = CreateWidget<UMyUI>(Controller, UIGameClass);
        if (UI)
        {
            UI->AddToViewport();
        }
    }

    // Use the grid placed in the level if there is one, otherwise spawn it.
    if (!GridManager)
    {
        FActorSpawnParameters SpawnParams;
        GridManager = GetWorld()->SpawnActor<AGridManager>(AGridManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    SetupUnits();

    UpdateHealthUI();

    PerformCoinToss();

    StartPlacementPhase();

    SetTopDownView();

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bEnableClickEvents = true;
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;
    }
}

void AGameManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Esc quits the game (handy in fullscreen, where there's no window X button).
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && PC->WasInputKeyJustPressed(EKeys::Escape))
        {
            UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
            return;
        }
    }

    HandleHoverCell();
    // Input is polled here instead of bound, and routed based on phase and whose turn it is.
    if (CurrentPhase == EGamePhase::EPlacement)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (bIsPlayerTurn && PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
        {
            HandleFloorClick();
        }
        else if (!bIsPlayerTurn)
        {
            AutoPlaceOpponentUnit();
        }
    }
    else if (CurrentPhase == EGamePhase::ETurn)
    {
        if (bIsPlayerTurn)
        {
            APlayerController* PC = GetWorld()->GetFirstPlayerController();
            if (PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
            {
                HandleFloorClickDuringTurn();
            }
        }
        else
        {
            if (bAiTurnPending)
            {
                PerformAiTurn();
                bAiTurnPending = false;
            }
            
        }
    }
}

// Placement phase: drop the next player unit on the clicked cell.
void AGameManager::HandleFloorClick()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !GridManager)
        return;

    // Ray from the mouse cursor into the scene to find which cell was clicked.
    FVector WorldLocation, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector Start = WorldLocation;
        FVector End = Start + WorldDirection * 10000.f;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
        {
            if (Hit.Component == GridManager->FloorMeshInstances)
            {
                FVector HitLocation = Hit.Location;
                FVector GridOffset = FVector(GridManager->GridSize * GridManager->CellSize * 0.5f,
                    GridManager->GridSize * GridManager->CellSize * 0.5f, 0.f);
                float AdjustedX = HitLocation.X + GridOffset.X;
                float AdjustedY = HitLocation.Y + GridOffset.Y;
                // RoundToInt: instances are centered on the cell, so map to the nearest center.
                int32 CellX = FMath::RoundToInt(AdjustedX / GridManager->CellSize);
                int32 CellY = FMath::RoundToInt(AdjustedY / GridManager->CellSize);

                 if (GridManager->IsCellWalkable(CellX, CellY))
                {
                    if (PlayerUnitsPlaced < UnitsToPlacePerSide)
                    {
                        ABaseUnit* UnitToPlace = PlayerUnits[PlayerUnitsPlaced];
                        bool bPlaced = PlaceUnit(UnitToPlace, CellX, CellY, true);
                        
                    }
                    else
                    {
                        if (GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Tutte le tue unit� sono state posizionate."));
                        }
                    }
                }
                else
                {
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Cella non disponibile!"));
                    }
                }
            }
        }
    }
}

void AGameManager::SetupUnits()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ABaseUnit* PlayerSniper = World->SpawnActor<ABaseUnit>(ASniper::StaticClass(), FVector(-500.f, 0.f, 310.f), FRotator::ZeroRotator);
    ABaseUnit* PlayerBrawler = World->SpawnActor<ABaseUnit>(ABrawler::StaticClass(), FVector(-500.f, 100.f, 310.f), FRotator::ZeroRotator);

    // All four units are spawned off-board and hidden; they show up once placed.
    // Stats (movement, range, damage, health) are set in the ASniper/ABrawler
    // constructors: here we only set ownership, visibility and material.
    if (PlayerSniper)
    {
        PlayerSniper->bIsPlayerUnit = true;
        PlayerUnits.Add(PlayerSniper);
        PlayerSniper->SetActorHiddenInGame(true);
    }
    if (PlayerBrawler)
    {
        PlayerBrawler->bIsPlayerUnit = true;
        PlayerUnits.Add(PlayerBrawler);
        PlayerBrawler->SetActorHiddenInGame(true);
    }

    ABaseUnit* OpponentSniper = World->SpawnActor<ABaseUnit>(ASniper::StaticClass(), FVector(500.f, 0.f, 310.f), FRotator::ZeroRotator);
    ABaseUnit* OpponentBrawler = World->SpawnActor<ABaseUnit>(ABrawler::StaticClass(), FVector(500.f, 100.f, 310.f), FRotator::ZeroRotator);

    // Same units, red textures for the AI side.
    UMaterialInterface* OpponentMaterialSniper = LoadObject<UMaterialInterface>(nullptr, TEXT("Material'/Game/texture/Soldier1_Redd.Soldier1_Redd'"));
    UMaterialInterface* OpponentMaterialBrawler = LoadObject<UMaterialInterface>(nullptr, TEXT("Material'/Game/texture/Soldier2_Redd.Soldier2_Redd'"));

    if (OpponentSniper)
    {
        OpponentSniper->bIsPlayerUnit = false;
        OpponentUnits.Add(OpponentSniper);
        OpponentSniper->SetActorHiddenInGame(true);

        if (OpponentMaterialSniper)
        {
            OpponentSniper->SetOpponentMaterial(OpponentMaterialSniper);
        }
    }
    if (OpponentBrawler)
    {
        OpponentBrawler->bIsPlayerUnit = false;
        OpponentUnits.Add(OpponentBrawler);
        OpponentBrawler->SetActorHiddenInGame(true);

        if (OpponentMaterialBrawler)
        {
            OpponentBrawler->SetOpponentMaterial(OpponentMaterialBrawler);
        }
    }

    PlayerUnitsPlaced = 0;
    OpponentUnitsPlaced = 0;
}

void AGameManager::PerformCoinToss()
{
    bPlayerWinsCoinToss = FMath::RandBool();
    FString TossResult = bPlayerWinsCoinToss ? TEXT("Player") : TEXT("Opponent");

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Risultato lancio moneta: %s"), *TossResult));
    }

    // Also show the result in the widget (move history), not just as an on-screen message.
    LogAction(FString::Printf(TEXT("Lancio moneta: inizia %s"), *TossResult));
}

void AGameManager::StartPlacementPhase()
{
    // Whoever wins the coin toss places first.
    CurrentPhase = EGamePhase::EPlacement;
    bIsPlayerTurn = bPlayerWinsCoinToss;
    FString TurnStr = bIsPlayerTurn ? TEXT("Player") : TEXT("Opponent");

    if (UI)
    {
        UI->UpdateTurnText(FString::Printf(TEXT("Turno: %s (Posizionamento)"), *TurnStr));
        // The End Turn button must do nothing during placement.
        UI->SetEndTurnButtonEnabled(false);
    }
}

bool AGameManager::PlaceUnit(ABaseUnit* Unit, int32 X, int32 Y, bool bIsPlayer)
{
    if (!Unit || !GridManager)
        return false;

    if (!GridManager->IsCellWalkable(X, Y))
    {
        return false;
    }

    Unit->CurrentX = X;
    Unit->CurrentY = Y;
    GridManager->SetCellOccupied(X, Y, true);

    FVector NewLocation = GridManager->GetCellWorldLocation(X, Y);
    NewLocation.Z += 1.f;

    Unit->SetActorLocation(NewLocation);
    Unit->SetActorHiddenInGame(false); 

    FString PlayerId = bIsPlayer ? TEXT("HP") : TEXT("AI");
    FString UnitId = Unit->IsA(ASniper::StaticClass()) ? TEXT("S") : TEXT("B");
    FString CellId = GridManager->GetCellIdentifier(X, Y);
    LogAction(FString::Printf(TEXT("%s: %s posizionato in %s"), *PlayerId, *UnitId, *CellId));

    if (bIsPlayer)
    {
        PlayerUnitsPlaced++;
    }
    else
    {
        OpponentUnitsPlaced++;
    }

    // Sides alternate one unit at a time during placement.
    bIsPlayerTurn = !bIsPlayerTurn;

    if (CurrentPhase == EGamePhase::EPlacement && UI)
    {
        FString TurnStr = bIsPlayerTurn ? TEXT("Player") : TEXT("Opponent");
        UI->UpdateTurnText(FString::Printf(TEXT("Turno: %s (Posizionamento)"), *TurnStr));
    }

    if (PlayerUnitsPlaced >= UnitsToPlacePerSide && OpponentUnitsPlaced >= UnitsToPlacePerSide)
    {
        EndPlacementPhase();
    }
    return true;
}

void AGameManager::EndPlacementPhase()
{
    // The coin toss winner also gets the first real turn.
    CurrentPhase = EGamePhase::ETurn;
    bIsPlayerTurn = bPlayerWinsCoinToss;
    if (UI)
    {
        FString TurnStr = bIsPlayerTurn ? TEXT("Player") : TEXT("Opponent");
        UI->UpdateTurnText(FString::Printf(TEXT("Turno: %s"), *TurnStr));
    }
    StartTurnPhase();
}

void AGameManager::StartTurnPhase()
{
    if (UI)
    {
        FString TurnText = bIsPlayerTurn ? TEXT("Turno: Player") : TEXT("Turno: Opponent");
        UI->UpdateTurnText(TurnText);
        // The End Turn button is only active during the player's turn.
        UI->SetEndTurnButtonEnabled(bIsPlayerTurn);
    }

    // When the player's turn starts, clear the red AI range highlight shown during the
    // opponent's turn, so it doesn't stay on screen.
    if (bIsPlayerTurn && GridManager)
    {
        GridManager->ClearMovementRange();
    }

    for (ABaseUnit* Unit : PlayerUnits)
    {
        if (Unit)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }
    for (ABaseUnit* Unit : OpponentUnits)
    {
        if (Unit)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }

    // If the turn that just started belongs to the AI, start it automatically. Covers both
    // later turns and the first turn after placement when the player loses the coin toss.
    if (CurrentPhase == EGamePhase::ETurn && !bIsPlayerTurn)
    {
        bAiTurnPending = true;
    }
}

void AGameManager::EndTurnPhase()
{
    CheckGameOver(); 

    bIsPlayerTurn = !bIsPlayerTurn;

    for (ABaseUnit* Unit : PlayerUnits)
    {
        if (Unit)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }
    for (ABaseUnit* Unit : OpponentUnits)
    {
        if (Unit)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }

    CurrentPhase = EGamePhase::ETurn;
}

void AGameManager::NextTurn()
{
    EndTurnPhase();
    StartTurnPhase();
}

void AGameManager::CheckGameOver()
{
    bool bPlayerAlive = false;
    bool bOpponentAlive = false;

    // This can run from inside ApplyDamageAmount, right before Destroy(), so
    // units that are dying must be skipped explicitly.
    for (ABaseUnit* Unit : PlayerUnits)
    {
        if (!Unit || static_cast<AActor*>(Unit)->IsActorBeingDestroyed() || !Unit->IsAlive())
            continue;
        bPlayerAlive = true;
        break;
    }
    for (ABaseUnit* Unit : OpponentUnits)
    {
        if (!Unit || static_cast<AActor*>(Unit)->IsActorBeingDestroyed() || !Unit->IsAlive())
            continue;
        bOpponentAlive = true;
        break;
    }

    FString Result;
    if (!bPlayerAlive && !bOpponentAlive)
    {
        Result = TEXT("DRAW");
    }
    else if (!bPlayerAlive)
    {
        Result = TEXT("GAME OVER");
    }
    else if (!bOpponentAlive)
    {
        Result = TEXT("WIN");
    }
    else
    {
        return; // Game still running.
    }

    CurrentPhase = EGamePhase::EEnd;

    // Hide the board (grid, obstacles, highlights and all units) but KEEP the
    // background (BackgroundMesh) visible, so at the end only the background and
    // the end panel with the result and buttons are left.
    if (GridManager)
    {
        if (GridManager->FloorMeshInstances)     GridManager->FloorMeshInstances->SetHiddenInGame(true);
        if (GridManager->ObstacleMeshInstances1) GridManager->ObstacleMeshInstances1->SetHiddenInGame(true);
        if (GridManager->ObstacleMeshInstances2) GridManager->ObstacleMeshInstances2->SetHiddenInGame(true);
        if (GridManager->ObstacleMeshInstances3) GridManager->ObstacleMeshInstances3->SetHiddenInGame(true);
        if (GridManager->HighlightMeshInstances) GridManager->HighlightMeshInstances->SetHiddenInGame(true);
    }
    for (ABaseUnit* Unit : PlayerUnits)
    {
        if (IsValid(Unit))
        {
            Unit->SetActorHiddenInGame(true);
        }
    }
    for (ABaseUnit* Unit : OpponentUnits)
    {
        if (IsValid(Unit))
        {
            Unit->SetActorHiddenInGame(true);
        }
    }

    if (UI)
    {
        UI->ShowEndGameScreen(Result);
    }
}

void AGameManager::LogAction(const FString& Action)
{
    MoveHistory.Add(Action);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Action);

    if (UI)
    {
        UI->AddMoveEntry(Action);
    }
}

bool AGameManager::ExecuteMove(ABaseUnit* Unit, int32 DestX, int32 DestY)
{
    if (!Unit || !GridManager)
        return false;

    // A unit that's already moving (animation running) can't take a new order:
    // prevents a double move before bHasMoved is set at the end of the animation.
    if (Unit->bIsMoving)
    {
        return false;
    }
    if (Unit->bHasMoved)
    {
        return false;
    }
    if (Unit->bHasAttacked)
    {
        return false;
    }

    TArray<FIntPoint> Path = GridManager->FindPathBFS(Unit->CurrentX, Unit->CurrentY, DestX, DestY);
    if (Path.Num() == 0)
    {
        return false;
    }

    // The logical position updates right away; the mesh catches up during the animation.
    FString Origin = GridManager->GetCellIdentifier(Unit->CurrentX, Unit->CurrentY);
    GridManager->SetCellOccupied(Unit->CurrentX, Unit->CurrentY, false);
    GridManager->SetCellOccupied(DestX, DestY, true);

    Unit->CurrentX = DestX;
    Unit->CurrentY = DestY;

    Unit->StartSmoothMovement(Path, GridManager);

    FString UnitId = Unit->IsA(ASniper::StaticClass()) ? "S" : "B";
    FString Side = Unit->bIsPlayerUnit ? "HP" : "AI";
    FString Destination = GridManager->GetCellIdentifier(DestX, DestY);
    LogAction(FString::Printf(TEXT("%s: %s %s -> %s"), *Side, *UnitId, *Origin, *Destination));

    return true;
}

bool AGameManager::ExecuteAttack(ABaseUnit* Attacker, ABaseUnit* Target)
{
    if (!Attacker || !Target || !GridManager)
        return false;

    // Grab the target's cell before attacking: PerformAttack may destroy the Target.
    const FString TargetCell = GridManager->GetCellIdentifier(Target->CurrentX, Target->CurrentY);

    bool bAttacked = Attacker->PerformAttack(Target, GridManager);
    if (bAttacked)
    {
        FString Side = Attacker->bIsPlayerUnit ? "HP" : "AI";
        FString UnitId = Attacker->IsA(ASniper::StaticClass()) ? "S" : "B";

        int32 Damage = Attacker->LastAttackDamage;
        LogAction(FString::Printf(TEXT("%s: %s %s %d"), *Side, *UnitId, *TargetCell, Damage));

        UpdateHealthUI();

        CheckIfActionsFinished();
    }
    return bAttacked;
}

void AGameManager::SetTopDownView()
{
    // Fixed camera straight above the board center, looking down.
    FVector Center = FVector(0.f, 0.f, 300.f);

    FVector CameraLocation = Center + FVector(0.f, 0.f, 2500.f);
    FRotator CameraRotation = FRotator(-90.f, 0.f, -90.f);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACameraActor* TopDownCamera = GetWorld()->SpawnActor<ACameraActor>(CameraLocation, CameraRotation, SpawnParams);
        if (TopDownCamera)
        {
            PC->SetViewTarget(TopDownCamera);
        }
    }
}

void AGameManager::OnUnitClicked(ABaseUnit* ClickedUnit)
{
    // Clicking one of our units selects it; clicking an enemy with a unit selected attacks it.
    if (PlayerUnits.Contains(ClickedUnit))
    {
        // Second click on the same unit: deselect and hide the range (as per spec).
        if (SelectedUnit == ClickedUnit)
        {
            SelectedUnit->SetSelected(false);
            SelectedUnit = nullptr;
            if (GridManager)
            {
                GridManager->ClearMovementRange();
            }
            return;
        }

        if (SelectedUnit && SelectedUnit != ClickedUnit)
        {
            SelectedUnit->SetSelected(false);
        }

        SelectedUnit = ClickedUnit;
        SelectedUnit->SetSelected(true);

        if (GridManager)
        {
            GridManager->SetHighlightColor(FLinearColor::White);
            GridManager->HighlightCell(SelectedUnit->CurrentX, SelectedUnit->CurrentY);
            GridManager->SetHighlightColor(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));
            GridManager->HighlightMovementRange(SelectedUnit->CurrentX, SelectedUnit->CurrentY, SelectedUnit->MaxMovement, FLinearColor::Blue);
        }
        HighlightEnemiesInRange(SelectedUnit);
    }
    else 
    {
        if (SelectedUnit)
        {
            ExecuteAttack(SelectedUnit, ClickedUnit);
            SelectedUnit->SetSelected(false);
            SelectedUnit = nullptr;
            if (GridManager)
            {
                GridManager->ClearMovementRange();
            }
        }
    }
}

void AGameManager::AutoPlaceOpponentUnit()
{
    if (!GridManager || OpponentUnitsPlaced >= UnitsToPlacePerSide)
    {
        return;
    }

    ABaseUnit* UnitToPlace = OpponentUnits[OpponentUnitsPlaced];
    if (!UnitToPlace)
    {
        return;
    }

    // Pick random cells until we find a free one. Capped so it can never spin forever.
    const int32 MaxAttempts = 100;
    int32 Attempt = 0;
    int32 CellX = -1, CellY = -1;
    while (Attempt < MaxAttempts)
    {
        int32 X = FMath::RandRange(0, GridManager->GridSize - 1);
        int32 Y = FMath::RandRange(0, GridManager->GridSize - 1);
        if (GridManager->IsCellWalkable(X, Y))
        {
            CellX = X;
            CellY = Y;
            break;
        }
        Attempt++;
    }

    if (CellX == -1)
    {
        return;
    }

    bool bPlaced = PlaceUnit(UnitToPlace, CellX, CellY, false);
    if (bPlaced)
    {
        
        if (GridManager)
        {
            GridManager->HighlightMovementRange(UnitToPlace->CurrentX, UnitToPlace->CurrentY, UnitToPlace->MaxMovement, FLinearColor::Red);
        }
    }
}

// Turn phase: move the selected unit to the clicked cell, if it's within range.
void AGameManager::HandleFloorClickDuringTurn()
{
    if (!SelectedUnit || !GridManager)
        return;

    if (SelectedUnit->bIsMoving)
    {
        return;
    }
    if (SelectedUnit->bHasMoved)
    {
        return;
    }
    if (SelectedUnit->bHasAttacked)
    {
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
        return;

    FVector WorldLocation, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector Start = WorldLocation;
        FVector End = Start + WorldDirection * 10000.f;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
        {
            if (Hit.Component == GridManager->FloorMeshInstances)
            {
                FVector HitLocation = Hit.Location;
                FVector GridOffset(
                    GridManager->GridSize * GridManager->CellSize * 0.5f,
                    GridManager->GridSize * GridManager->CellSize * 0.5f,
                    0.f
                );

                float AdjustedX = HitLocation.X + GridOffset.X;
                float AdjustedY = HitLocation.Y + GridOffset.Y;
                // RoundToInt: instances are centered on the cell, so map to the nearest center.
                int32 CellX = FMath::RoundToInt(AdjustedX / GridManager->CellSize);
                int32 CellY = FMath::RoundToInt(AdjustedY / GridManager->CellSize);

                TArray<FIntPoint> Path;
                if (SelectedUnit->bIsPlayerUnit)
                {
                    Path = GridManager->FindPathBFS(
                        SelectedUnit->CurrentX,
                        SelectedUnit->CurrentY,
                        CellX,
                        CellY
                    );
                }
                else
                {
                    Path = GridManager->FindPathAStar(
                        SelectedUnit->CurrentX,
                        SelectedUnit->CurrentY,
                        CellX,
                        CellY
                    );
                }

                if (Path.Num() == 0)
                {
                    return;
                }

                // Path includes the start cell, so steps = length - 1.
                int32 Distance = Path.Num() - 1;
                if (Distance >= 0 && Distance <= SelectedUnit->MaxMovement)
                {
                    bool bMoved = ExecuteMove(SelectedUnit, CellX, CellY);
                    if (bMoved)
                    {
                        GridManager->ClearMovementRange();
                    }
                }
            }
        }
    }
}

void AGameManager::CheckIfActionsFinished()
{
    bool bAllUnitsDone = true;
    if (bIsPlayerTurn) 
    {
        for (ABaseUnit* Unit : PlayerUnits)
        {
            if (!Unit || !Unit->IsAlive())
                continue;
            if (Unit->bIsMoving || !(Unit->bHasMoved || Unit->bHasAttacked))
            {
                bAllUnitsDone = false;
                break;
            }
        }
    }
    else 
    {
        for (ABaseUnit* Unit : OpponentUnits)
        {
            if (!Unit || !Unit->IsAlive())
                continue;
            if (Unit->bIsMoving || !(Unit->bHasMoved || Unit->bHasAttacked))
            {
                bAllUnitsDone = false;
                break;
            }
        }
    }

    // Only the AI turn ends by itself; the player always ends it with the button.
    if (bAllUnitsDone)
    {
        if (!bIsPlayerTurn)
        {
            NextTurn();
        }
    }
}

void AGameManager::HandleHoverCell()
{
    // Hover only works on the player's turn (placement or play): that way it doesn't
    // mess with the AI range highlight shown during the opponent's pause.
    if (!bIsPlayerTurn || CurrentPhase == EGamePhase::EEnd)
        return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !GridManager)
        return;

    // While a unit is selected its (blue) movement range stays on screen:
    // hover must not overwrite it, or the range highlight would disappear.
    if (SelectedUnit)
    {
        LastHoveredCell = FIntPoint(-1, -1);
        return;
    }

    FVector WorldLocation, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector Start = WorldLocation;
        FVector End = Start + WorldDirection * 10000.f;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
        {
            if (Hit.GetActor() && Hit.GetActor()->IsA(ABaseUnit::StaticClass()))
            {
                LastHoveredCell = FIntPoint(-1, -1);
                GridManager->ClearMovementRange();
                return;
            }

            if (Hit.Component == GridManager->FloorMeshInstances)
            {
                FVector HitLocation = Hit.Location;
                FVector GridOffset = FVector(GridManager->GridSize * GridManager->CellSize * 0.5f,
                    GridManager->GridSize * GridManager->CellSize * 0.5f, 0.f);
                int32 CellX = FMath::RoundToInt((HitLocation.X + GridOffset.X) / GridManager->CellSize);
                int32 CellY = FMath::RoundToInt((HitLocation.Y + GridOffset.Y) / GridManager->CellSize);

                FIntPoint HoveredCell(CellX, CellY);
                if (HoveredCell != LastHoveredCell)
                {
                    LastHoveredCell = HoveredCell;
                    GridManager->SetHighlightColor(FLinearColor::Yellow);
                    GridManager->HighlightCell(CellX, CellY);
                }
            }
            else
            {
                LastHoveredCell = FIntPoint(-1, -1);
                GridManager->ClearMovementRange();
            }
        }
    }
}

void AGameManager::HighlightEnemiesInRange(ABaseUnit* UnitSelezionata)
{
    if (!UnitSelezionata || !GridManager)
        return;

    for (ABaseUnit* EnemyUnit : OpponentUnits)
    {
        if (EnemyUnit && EnemyUnit->IsAlive())
        {
            bool bInRange = false;
            if (UnitSelezionata->AttackType == EAttackType::Ranged)
            {
                int32 Distance = FMath::Abs(UnitSelezionata->CurrentX - EnemyUnit->CurrentX) +
                    FMath::Abs(UnitSelezionata->CurrentY - EnemyUnit->CurrentY);
                bInRange = (Distance <= UnitSelezionata->AttackRange);
            }
            else 
            {
                int32 ManhattanDistance = FMath::Abs(UnitSelezionata->CurrentX - EnemyUnit->CurrentX) +
                    FMath::Abs(UnitSelezionata->CurrentY - EnemyUnit->CurrentY);
                bInRange = (ManhattanDistance <= UnitSelezionata->AttackRange);
            }
            // Debug boxes are a quick way to mark targets without touching the shared
            // highlight mesh (which is already showing the movement range).
            if (bInRange)
            {
                FVector EnemyCellLocation = GridManager->GetCellWorldLocation(EnemyUnit->CurrentX, EnemyUnit->CurrentY);
                EnemyCellLocation.Z += 5.f; 

                float CellGap = 5.f;
                float ScaleFactor = (GridManager->CellSize - CellGap) / GridManager->CellSize;
                float EffectiveCellSize = GridManager->CellSize * ScaleFactor;

                FVector BoxExtent(EffectiveCellSize * 0.5f, EffectiveCellSize * 0.5f, 10.f);
                DrawDebugBox(GetWorld(), EnemyCellLocation, BoxExtent, FColor::Red, false, 2.f, 0, 5.f);
            }
        }
    }
}

void AGameManager::PerformAiTurn()
{
    // Requirement: during the AI turn, its units' movement range is highlighted on the
    // grid (automatically, without the player selecting anything).
    // The range is shown while the units are still idle; after a short pause it's
    // cleared and the units make their moves (see ExecuteAiActions).
    if (GridManager)
    {
        GridManager->HighlightOpponentMovementRange(OpponentUnits, FLinearColor::Red);
    }

    const float HighlightDelay = 0.9f;
    GetWorldTimerManager().SetTimer(
        AiHighlightTimerHandle, this, &AGameManager::ExecuteAiActions, HighlightDelay, false);
}

void AGameManager::ExecuteAiActions()
{
    // Clear the range highlight before moving the units.
    if (GridManager)
    {
        GridManager->ClearMovementRange();
    }

    for (ABaseUnit* OppUnit : OpponentUnits)
    {
        if (!OppUnit || !OppUnit->IsAlive() || OppUnit->bHasMoved)
        {
            continue;
        }

        // Player units still alive.
        TArray<ABaseUnit*> AliveTargets;
        for (ABaseUnit* P : PlayerUnits)
        {
            if (P && P->IsAlive())
            {
                AliveTargets.Add(P);
            }
        }
        if (AliveTargets.Num() == 0)
        {
            continue;
        }

        // Target is picked at RANDOM (requirement: AI with random actions).
        ABaseUnit* Target = AliveTargets[FMath::RandRange(0, AliveTargets.Num() - 1)];

        // Movement: 50% of the time the AI closes in on the enemy using A*
        // (requirement: optimized algorithm), otherwise it moves to a random
        // reachable cell (requirement: random movement).
        const bool bMoveTowardsEnemy = FMath::RandBool();

        if (bMoveTowardsEnemy)
        {
            FIntPoint AdjacentCell = GridManager->GetNearestWalkableAdjacentCell(Target->CurrentX, Target->CurrentY);
            if (AdjacentCell.X != -1)
            {
                TArray<FIntPoint> Path = GridManager->FindPathAStar(
                    OppUnit->CurrentX, OppUnit->CurrentY, AdjacentCell.X, AdjacentCell.Y);
                if (Path.Num() > 1)
                {
                    // Walk as far along the path as this unit's movement allows.
                    int32 MaxSteps = FMath::Min(OppUnit->MaxMovement, Path.Num() - 1);
                    FIntPoint Destination = Path[MaxSteps];
                    ExecuteMove(OppUnit, Destination.X, Destination.Y);
                }
            }
        }
        else
        {
            TArray<FIntPoint> Reachable = GridManager->GetReachableCells(
                OppUnit->CurrentX, OppUnit->CurrentY, OppUnit->MaxMovement);
            if (Reachable.Num() > 0)
            {
                FIntPoint Destination = Reachable[FMath::RandRange(0, Reachable.Num() - 1)];
                ExecuteMove(OppUnit, Destination.X, Destination.Y);
            }
        }

        // Attack: hit a random enemy among those in range (random action).
        // Range is checked against the new cell, since ExecuteMove updates it immediately.
        if (!OppUnit->bHasAttacked && OppUnit->IsAlive())
        {
            TArray<ABaseUnit*> InRange;
            for (ABaseUnit* P : PlayerUnits)
            {
                if (!P || !P->IsAlive())
                {
                    continue;
                }
                int32 Dist = FMath::Abs(OppUnit->CurrentX - P->CurrentX) +
                    FMath::Abs(OppUnit->CurrentY - P->CurrentY);
                if (Dist <= OppUnit->AttackRange)
                {
                    InRange.Add(P);
                }
            }
            if (InRange.Num() > 0)
            {
                ABaseUnit* AttackTarget = InRange[FMath::RandRange(0, InRange.Num() - 1)];
                ExecuteAttack(OppUnit, AttackTarget);
            }
        }
    }

    // Makes sure the AI turn always ends: every living unit that is NOT moving
    // is done with its actions (even if it couldn't move or attack).
    // Without this, a stuck unit left the turn hanging forever.
    for (ABaseUnit* OppUnit : OpponentUnits)
    {
        if (OppUnit && OppUnit->IsAlive() && !OppUnit->bIsMoving)
        {
            OppUnit->bHasMoved = true;
            OppUnit->bHasAttacked = true;
        }
    }
    CheckIfActionsFinished();
}

// HUD slots 0-1 are the player's Sniper/Brawler, 2-3 the opponent's.
void AGameManager::UpdateHealthUI()
{
    if (UI)
    {
        if (PlayerUnits.Num() >= 1 && PlayerUnits[0])
        {
            UI->UpdateUnitHealth(0, PlayerUnits[0]->Health, PlayerUnits[0]->MaxHealth);
        }
        if (PlayerUnits.Num() >= 2 && PlayerUnits[1])
        {
            UI->UpdateUnitHealth(1, PlayerUnits[1]->Health, PlayerUnits[1]->MaxHealth);
        }
        if (OpponentUnits.Num() >= 1 && OpponentUnits[0])
        {
            UI->UpdateUnitHealth(2, OpponentUnits[0]->Health, OpponentUnits[0]->MaxHealth);
        }
        if (OpponentUnits.Num() >= 2 && OpponentUnits[1])
        {
            UI->UpdateUnitHealth(3, OpponentUnits[1]->Health, OpponentUnits[1]->MaxHealth);
        }
    }
}

