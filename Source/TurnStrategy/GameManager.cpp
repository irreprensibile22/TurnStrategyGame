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

AGameManager::AGameManager()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentPhase = EGamePhase::EPlacement;
    bPlayerWinsCoinToss = false;
    bIsPlayerTurn = false;
    UnitsToPlacePerSide = 2;
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

    HandleHoverCell();
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

void AGameManager::HandleFloorClick()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !GridManager)
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
                FVector GridOffset = FVector(GridManager->GridSize * GridManager->CellSize * 0.5f,
                    GridManager->GridSize * GridManager->CellSize * 0.5f, 0.f);
                float AdjustedX = HitLocation.X + GridOffset.X;
                float AdjustedY = HitLocation.Y + GridOffset.Y;
                // RoundToInt: le istanze sono centrate sulla cella, quindi si mappa al centro piu' vicino.
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

    // Le statistiche (movimento, range, danno, vita) sono impostate nei costruttori
    // di ASniper/ABrawler: qui si configura solo appartenenza, visibilita' e materiale.
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

    // Mostra l'esito anche nel widget (storico mosse), oltre al messaggio a schermo.
    LogAction(FString::Printf(TEXT("Lancio moneta: inizia %s"), *TossResult));
}

void AGameManager::StartPlacementPhase()
{
    CurrentPhase = EGamePhase::EPlacement;
    bIsPlayerTurn = bPlayerWinsCoinToss;
    FString TurnStr = bIsPlayerTurn ? TEXT("Player") : TEXT("Opponent");

    if (UI)
    {
        UI->UpdateTurnText(FString::Printf(TEXT("Turno: %s (Posizionamento)"), *TurnStr));
        // In posizionamento il pulsante Termina Turno non deve avere effetto.
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
        // Il pulsante Termina Turno e' attivo solo nel turno di gioco del player.
        UI->SetEndTurnButtonEnabled(bIsPlayerTurn);
    }

    // All'inizio del turno del giocatore rimuove l'evidenziazione (rossa) del range dell'AI
    // mostrata durante il turno avversario, cosi' non resta a schermo nel turno del player.
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

    // Se il turno appena iniziato e' dell'AI, avvialo automaticamente. Copre sia i turni
    // successivi sia il primo turno dopo il posizionamento quando il player perde il coin toss.
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
        return; // Partita ancora in corso.
    }

    CurrentPhase = EGamePhase::EEnd;

    // Nasconde la scena di gioco (griglia, ostacoli, evidenziazioni e tutte le unita'),
    // MANTENENDO visibile lo sfondo di gioco (BackgroundMesh), cosi' a fine partita
    // restano lo sfondo e il pannello di fine con esito e pulsanti.
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

    // Un'unita' gia' in movimento (animazione in corso) non puo' ricevere un nuovo ordine:
    // evita il doppio movimento prima che bHasMoved venga impostato a fine animazione.
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

    // Cella del bersaglio catturata prima dell'attacco: PerformAttack puo' distruggere il Target.
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
    if (PlayerUnits.Contains(ClickedUnit))
    {
        // Secondo click sulla stessa unita': deseleziona e nasconde il range (come da specifiche).
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
                // RoundToInt: le istanze sono centrate sulla cella, quindi si mappa al centro piu' vicino.
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
    // L'hover agisce solo quando e' il turno del giocatore (posizionamento o gioco): cosi'
    // non disturba l'evidenziazione del range dell'AI mostrata nella pausa del turno avversario.
    if (!bIsPlayerTurn || CurrentPhase == EGamePhase::EEnd)
        return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !GridManager)
        return;

    // Quando un'unita' e' selezionata resta mostrato il suo range di movimento (blu):
    // l'hover non deve sovrascriverlo, altrimenti l'evidenziazione del range sparirebbe.
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
    // Requisito: durante il turno dell'AI si evidenzia sulla griglia il range di
    // movimento delle sue unita' (automaticamente, senza selezione da parte del player).
    // Si mostra il range con le pedine ancora ferme; dopo una breve pausa il range viene
    // rimosso e le unita' eseguono le loro mosse (vedi ExecuteAiActions).
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
    // Rimuove l'evidenziazione del range prima di muovere le unita'.
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

        // Elenco delle unita' del player ancora vive.
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

        // Scelta del bersaglio in modo RANDOM (requisito: AI con azioni random).
        ABaseUnit* Target = AliveTargets[FMath::RandRange(0, AliveTargets.Num() - 1)];

        // Movimento: con probabilita' 50% l'IA si avvicina al nemico usando A*
        // (requisito: algoritmo ottimizzato), altrimenti si muove verso una cella
        // raggiungibile scelta a caso (requisito: movimento random).
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

        // Attacco: colpisce un nemico scelto a caso tra quelli nel range (azione random).
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

    // Garanzia di terminazione del turno AI: ogni unita' viva che NON si sta muovendo
    // ha concluso le proprie azioni (anche se non ha potuto muoversi o attaccare).
    // Senza questo, un'unita' bloccata lasciava il turno appeso all'infinito.
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

