#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyUI.h"
#include "GameManager.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    EPlacement UMETA(DisplayName = "Placement"),
    ETurn      UMETA(DisplayName = "Turn"),
    EEnd       UMETA(DisplayName = "End")
};

UCLASS()
class PROGETTOPAA_API AGameManager : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGameManager();

private:
    ABaseUnit* SelectedUnit;

    FIntPoint LastHoveredCell = FIntPoint(-1, -1);



protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    EGamePhase CurrentPhase;

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

    class ABaseUnit* FindNearestPlayerUnit(class ABaseUnit* EnemyUnit);


    UPROPERTY()
    UMyUI* UI;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UMyUI> UIGameClass;

    void UpdateHealthUI();

    bool bAiTurnPending;


};
