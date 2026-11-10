#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUi.generated.h"

class UTextBlock;
class UButton; 
class UScrollBox;
class AGameManager;

// C++ side of the HUD. The layout is in MyUI_Blueprint; the BindWidget names
// below must match the widget names there exactly.
UCLASS()
class TURNSTRATEGY_API UMyUI : public UUserWidget
{
    GENERATED_BODY()

public:
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateTurnText(const FString& NewTurnText);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateUnitHealth(int32 UnitIndex, int32 CurrentHealth, int32 MaxHealth);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void AddMoveEntry(const FString& MoveText);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowEndGameScreen(const FString& EndMessage);

    // Enables/disables the End Turn button (off during placement and the AI turn).
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetEndTurnButtonEnabled(bool bEnabled);

    UFUNCTION()
    void OnQuitButtonClicked();

    // Restarts the match by reloading the current level (button on the end-game panel).
    UFUNCTION()
    void OnPlayAgainButtonClicked();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UTextBlock* TurnTextBlock;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UButton* EndTurnButton;

    // Optional text label for the End Turn button, if it's a separate element in the
    // widget. Optional binding: no error if it doesn't exist.
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* EndTurnButtonTextBlock;

    // One health line per unit, same order as UpdateUnitHealth's index.
    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthStatusPedina1;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthStatusPedina2;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthStatusPedina3;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthStatusPedina4;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* MovesScrollBox;

    UPROPERTY(meta = (BindWidget))
    UWidget* EndGamePanel;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* EndGameTextBlock;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    class UButton* QuitButton;

    // Optional: as long as the button isn't in the widget, the binding won't error out.
    UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
    class UButton* PlayAgainButton;

    UFUNCTION()
    void OnEndTurnButtonClicked();

private:
    
    AGameManager* GameManagerRef;
};
