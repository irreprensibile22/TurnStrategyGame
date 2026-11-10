#include "MyUi.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameManager.h"

void UMyUI::UpdateTurnText(const FString& NewTurnText)
{
    if (TurnTextBlock)
    {
        TurnTextBlock->SetText(FText::FromString(NewTurnText));
    }
    
}

void UMyUI::NativeConstruct()
{
    Super::NativeConstruct();

    GameManagerRef = Cast<AGameManager>(UGameplayStatics::GetGameMode(this));

    // Hook up the buttons. PlayAgain is optional, so it may be null.
    if (EndTurnButton)
    {
        EndTurnButton->OnClicked.AddDynamic(this, &UMyUI::OnEndTurnButtonClicked);
    }
    
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UMyUI::OnQuitButtonClicked);
    }

    if (PlayAgainButton)
    {
        PlayAgainButton->OnClicked.AddDynamic(this, &UMyUI::OnPlayAgainButtonClicked);
    }
}

void UMyUI::OnEndTurnButtonClicked()
{
    if (GameManagerRef)
    {
        // The button only works during the player's turn: during the AI turn the
        // click is ignored so it doesn't re-trigger the opponent's logic.
        if (GameManagerRef->bIsPlayerTurn)
        {
            // NextTurn hands the turn to the AI; StartTurnPhase takes care of starting it.
            GameManagerRef->NextTurn();
        }
    }
}

void UMyUI::UpdateUnitHealth(int32 UnitIndex, int32 CurrentHealth, int32 MaxHealth)
{
    FString UnitName;
    switch (UnitIndex)
    {
    case 0:
        UnitName = TEXT("Player Sniper");
        break;
    case 1:
        UnitName = TEXT("Player Brawler");
        break;
    case 2:
        UnitName = TEXT("Opponent Sniper");
        break;
    case 3:
        UnitName = TEXT("Opponent Brawler");
        break;
    default:
        UnitName = TEXT("Unit");
        break;
    }

    FString HealthString = FString::Printf(TEXT("%s: %d / %d"), *UnitName, CurrentHealth, MaxHealth);

    switch (UnitIndex)
    {
    case 0:
        if (HealthStatusPedina1)
            HealthStatusPedina1->SetText(FText::FromString(HealthString));
        break;
    case 1:
        if (HealthStatusPedina2)
            HealthStatusPedina2->SetText(FText::FromString(HealthString));
        break;
    case 2:
        if (HealthStatusPedina3)
            HealthStatusPedina3->SetText(FText::FromString(HealthString));
        break;
    case 3:
        if (HealthStatusPedina4)
            HealthStatusPedina4->SetText(FText::FromString(HealthString));
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("UnitIndex fuori range: %d"), UnitIndex);
        break;
    }
}

void UMyUI::AddMoveEntry(const FString& MoveText)
{
    if (MovesScrollBox)
    {
        UTextBlock* NewEntry = NewObject<UTextBlock>(this, UTextBlock::StaticClass());
        if (NewEntry)
        {
            NewEntry->SetText(FText::FromString(MoveText));
            // Wrap the text inside the ScrollBox width instead of growing
            // horizontally and spilling over the grid.
            NewEntry->SetAutoWrapText(true);
            MovesScrollBox->AddChild(NewEntry);

            int32 ChildCount = MovesScrollBox->GetChildrenCount();
            if (ChildCount > 0)
            {
                UWidget* LastChild = MovesScrollBox->GetChildAt(ChildCount - 1);
                MovesScrollBox->ScrollToEnd();
            }

        }
    }
   
}

void UMyUI::SetEndTurnButtonEnabled(bool bEnabled)
{
    if (EndTurnButton)
    {
        EndTurnButton->SetIsEnabled(bEnabled);
    }
}

void UMyUI::ShowEndGameScreen(const FString& EndMessage)
{
    // Hide the in-game HUD: only the end-game panel stays visible.
    UWidget* GameplayWidgets[] = {
        TurnTextBlock, EndTurnButton, EndTurnButtonTextBlock, MovesScrollBox,
        HealthStatusPedina1, HealthStatusPedina2, HealthStatusPedina3, HealthStatusPedina4
    };
    for (UWidget* Widget : GameplayWidgets)
    {
        if (Widget)
        {
            Widget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (EndGameTextBlock)
    {
        EndGameTextBlock->SetText(FText::FromString(EndMessage));
    }

    if (EndGamePanel)
    {
        EndGamePanel->SetVisibility(ESlateVisibility::Visible);
    }
}

void UMyUI::OnQuitButtonClicked()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
    }
}

void UMyUI::OnPlayAgainButtonClicked()
{
    // Reload the current level: rebuilds grid, units and state, restarting the match.
    const FName CurrentLevel(*UGameplayStatics::GetCurrentLevelName(this, true));
    UGameplayStatics::OpenLevel(this, CurrentLevel);
}