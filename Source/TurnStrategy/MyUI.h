#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUi.generated.h"

class UTextBlock;
class UButton; 
class UScrollBox;
class AGameManager;

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

    // Abilita/disabilita il pulsante Termina Turno (disattivo in posizionamento e nel turno AI).
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetEndTurnButtonEnabled(bool bEnabled);

    UFUNCTION()
    void OnQuitButtonClicked();

    // Riavvia la partita ricaricando il livello corrente (pulsante del pannello di fine partita).
    UFUNCTION()
    void OnPlayAgainButtonClicked();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UTextBlock* TurnTextBlock;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UButton* EndTurnButton;

    // Eventuale label testuale del pulsante Termina Turno, se nel widget e' un elemento
    // separato dal pulsante. Opzionale: se non esiste, il binding non va in errore.
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* EndTurnButtonTextBlock;

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

    // Opzionale: finche' il pulsante non esiste nel widget, il binding non va in errore.
    UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
    class UButton* PlayAgainButton;

    UFUNCTION()
    void OnEndTurnButtonClicked();

private:
    
    AGameManager* GameManagerRef;
};
