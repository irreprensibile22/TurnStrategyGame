#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyUI.h"
#include "GameManager.generated.h"

// Game flow: units get placed first, then turns alternate until one side is wiped out.
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    EPlacement UMETA(DisplayName = "Placement"),
    ETurn      UMETA(DisplayName = "Turn"),
    EEnd       UMETA(DisplayName = "End")
};

// The game mode is the referee: it owns the turn state, handles player input,
// runs the AI and keeps the HUD in sync.
UCLASS()
class TURNSTRATEGY_API AGameManager : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGameManager();

private:
    UPROPERTY()
    ABaseUnit* SelectedUnit;

    // Cached so the hover highlight is only redrawn when the mouse changes cell.
    FIntPoint LastHoveredCell = FIntPoint(-1, -1);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    EGamePhase CurrentPhase;

    // Index 0 is always the Sniper, index 1 the Brawler (UpdateHealthUI relies on it).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    TArray<class ABaseUnit*> PlayerUnits;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    TArray<class ABaseUnit*> OpponentUnits;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
    class AGridManager* GridManager;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    bool bPlayerWinsCoinToss;

    int32 UnitsToPlacePerSide;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    TArray<FString> MoveHistory;

    int32 PlayerUnitsPlaced;
    int32 OpponentUnitsPlaced;

    void SetupUnits();
    void PerformCoinToss();
    void StartPlacementPhase();
    void EndPlacementPhase();
    void StartTurnPhase();
    void EndTurnPhase();
    void LogAction(const FString& Action);
    void AutoPlaceOpponentUnit();

public:

    void CheckGameOver();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    bool bIsPlayerTurn;

    UFUNCTION(BlueprintCallable, Category = "Game")
    bool PlaceUnit(class ABaseUnit* Unit, int32 X, int32 Y, bool bIsPlayer);

    UFUNCTION(BlueprintCallable, Category = "Game")
    void NextTurn();

    bool ExecuteMove(ABaseUnit* Unit, int32 DestX, int32 DestY);

    bool ExecuteAttack(ABaseUnit* Attacker, ABaseUnit* Target);

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetTopDownView();

    UFUNCTION()
    void OnUnitClicked(ABaseUnit* ClickedUnit);

    void HandleFloorClick();

    void HandleFloorClickDuringTurn();

    void CheckIfActionsFinished();

    void HandleHoverCell();

    void HighlightEnemiesInRange(ABaseUnit* SelectedUnit);

    void PerformAiTurn();

    // Actually runs the AI moves and attacks, after the short highlight pause.
    void ExecuteAiActions();

    // Timer that keeps the AI range highlighted (units still idle) before moving them.
    FTimerHandle AiHighlightTimerHandle;

    UPROPERTY()
    UMyUI* UI;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UMyUI> UIGameClass;

    void UpdateHealthUI();

    // Set when the AI turn starts; Tick picks it up and kicks off PerformAiTurn once.
    bool bAiTurnPending;

};
