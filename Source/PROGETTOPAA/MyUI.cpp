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

    if (EndTurnButton)
    {
        EndTurnButton->OnClicked.AddDynamic(this, &UMyUI::OnEndTurnButtonClicked);
    }
    
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UMyUI::OnQuitButtonClicked);
    }
}

void UMyUI::OnEndTurnButtonClicked()
{
    if (GameManagerRef)
    {
        // Il pulsante ha effetto solo durante il turno del giocatore: durante il turno
        // dell'AI il click viene ignorato per non ri-attivare la logica dell'avversario.
        if (GameManagerRef->bIsPlayerTurn)
        {
            GameManagerRef->NextTurn();
            GameManagerRef->bAiTurnPending = true;
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

void UMyUI::ShowEndGameScreen(const FString& EndMessage)
{
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