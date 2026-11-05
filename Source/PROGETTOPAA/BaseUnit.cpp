#include "BaseUnit.h"
#include "GridManager.h"
#include "Sniper.h"    
#include "Brawler.h"   
#include "GameManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> UnitMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (UnitMeshAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(UnitMeshAsset.Object);
    }
    VisualMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
    VisualMesh->SetWorldScale3D(FVector(0.9f, 0.9f, 1.f));

    VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
    VisualMesh->SetGenerateOverlapEvents(true);



    MaxMovement = 0;
    AttackRange = 0;
    MinDamage = 0;
    MaxDamage = 0;
    Health = 0;
    AttackType = EAttackType::Melee;
    CurrentX = 0;
    CurrentY = 0;
    bHasMoved = false;
    bHasAttacked = false;
    bIsSelected = false;
    MovementSpeed = 200.f; 
    bIsMoving = false;
    CurrentWaypointIndex = 0;
    LastAttackDamage = 0;




}

void ABaseUnit::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;
}

int32 ABaseUnit::Attack()
{
    return FMath::RandRange(MinDamage, MaxDamage);
}

void ABaseUnit::ApplyDamageAmount(int32 DamageAmount)
{
    Health -= DamageAmount;
    if (Health <= 0)
    {
        Health = 0;
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);

        AGameManager* GM = Cast<AGameManager>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GM)
        {
            GM->CheckGameOver();
        }

         Destroy();
    }
}

bool ABaseUnit::IsAlive() const
{
    return Health > 0;
}

bool ABaseUnit::PerformAttack(ABaseUnit* Target, AGridManager* GridManager)
{
    if (bHasAttacked)
    {
        return false;
    }
    if (!Target || !GridManager)
    {
        return false;
    }

    bool bInRange = false;
    if (AttackType == EAttackType::Ranged)
    {
        int32 Distance = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
        bInRange = (Distance <= AttackRange);
    }
    else
    {
        int32 ManhattanDistance = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
        bInRange = (ManhattanDistance <= AttackRange);
    }

    if (!bInRange)
    {
        return false;
    }

    // Calcola tipo/distanza del bersaglio PRIMA di applicare il danno:
    // ApplyDamageAmount puo' distruggere il Target, rendendone l'accesso successivo non sicuro.
    const int32 DistToTarget = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
    const bool bTargetIsSniper = Target->IsA(ASniper::StaticClass());
    const bool bTargetIsBrawler = Target->IsA(ABrawler::StaticClass());
    const int32 TargetX = Target->CurrentX;
    const int32 TargetY = Target->CurrentY;

    int32 Damage = Attack();
    LastAttackDamage = Damage;
    Target->ApplyDamageAmount(Damage);

    // Danno da contrattacco: solo se l'attaccante e' lo Sniper (Ranged) e il bersaglio
    // e' uno Sniper, oppure un Brawler a distanza 1.
    if (AttackType == EAttackType::Ranged)
    {
        if (bTargetIsSniper || (bTargetIsBrawler && (DistToTarget == 1)))
        {
            int32 CounterDamage = FMath::RandRange(1, 3);
            ApplyDamageAmount(CounterDamage);
        }
    }

    // Libera la cella solo se il bersaglio e' stato eliminato (IsValid copre il caso Destroy()).
    if (!IsValid(Target) || !Target->IsAlive())
    {
        GridManager->SetCellOccupied(TargetX, TargetY, false);
    }

    bHasAttacked = true;
    return true;
}

bool ABaseUnit::MoveTo(int32 DestinationX, int32 DestinationY, AGridManager* GridManager)
{

    if (bHasMoved)
    {
        return false;
    }
    if (!GridManager)
    {
        return false;
    }

    TArray<FIntPoint> Path = GridManager->FindPathBFS(CurrentX, CurrentY, DestinationX, DestinationY);

    if (Path.Num() > 0 && (Path.Num() - 1) <= MaxMovement)
    {
        if (!GridManager->IsCellWalkable(DestinationX, DestinationY))
        {
            return false;
        }

        GridManager->SetCellOccupied(CurrentX, CurrentY, false);
        GridManager->SetCellOccupied(DestinationX, DestinationY, true);

        CurrentX = DestinationX;
        CurrentY = DestinationY;

        FVector NewLocation = GridManager->GetCellWorldLocation(DestinationX, DestinationY);
        NewLocation.Z += 1.f;
        SetActorLocation(NewLocation);

        bHasMoved = true;
        return true;
    }
    else
    {
        return false;
    }
}






void ABaseUnit::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);
    AGameManager* GM = Cast<AGameManager>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM)
    {
        GM->OnUnitClicked(this);
    }
}





void ABaseUnit::SetOpponentMaterial(UMaterialInterface* NewMaterial)
{
    if (VisualMesh && NewMaterial)
    {
        VisualMesh->SetMaterial(0, NewMaterial);
    }
}




void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving && Waypoints.Num() > 0)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector TargetLocation = Waypoints[CurrentWaypointIndex];

        FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, MovementSpeed);
        SetActorLocation(NewLocation);

        if (FVector::Dist(NewLocation, TargetLocation) < 1.f) 
        {
            CurrentWaypointIndex++;
            if (CurrentWaypointIndex >= Waypoints.Num())
            {
                bIsMoving = false;
                Waypoints.Empty();
                CurrentWaypointIndex = 0;
                bHasMoved = true;
                
                AGameManager* GM = Cast<AGameManager>(UGameplayStatics::GetGameMode(GetWorld()));
                if (GM)
                {
                    GM->CheckIfActionsFinished(); 
                }
            }
        }
    }
}





void ABaseUnit::StartSmoothMovement(const TArray<FIntPoint>& Path, AGridManager* GridManager)
{
    if (!GridManager || Path.Num() == 0)
    {
        return;
    }
    Waypoints.Empty();
    for (const FIntPoint& GridPoint : Path)
    {
        FVector WorldPos = GridManager->GetCellWorldLocation(GridPoint.X, GridPoint.Y);
        WorldPos.Z += 1.f; 
        Waypoints.Add(WorldPos);
    }
    CurrentWaypointIndex = 0;
    bIsMoving = true;
}
