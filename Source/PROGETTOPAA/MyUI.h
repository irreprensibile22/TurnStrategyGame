#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUi.generated.h"

class UTextBlock;
class UButton; 
class UScrollBox;
class AGameManager;

UCLASS()
class PROGETTOPAA_API UMyUI : public UUserWidget
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

    UFUNCTION()
    void OnQuitButtonClicked();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UTextBlock* TurnTextBlock;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UButton* EndTurnButton;

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

    UFUNCTION()
    void OnEndTurnButtonClicked();

private:
    
    AGameManager* GameManagerRef;
};
