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

    // Units are just flat textured planes, slightly smaller than a cell.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> UnitMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (UnitMeshAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(UnitMeshAsset.Object);
    }
    VisualMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
    VisualMesh->SetWorldScale3D(FVector(0.9f, 0.9f, 1.f));

    // Needs to block traces, otherwise NotifyActorOnClicked never fires.
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
    VisualMesh->SetGenerateOverlapEvents(true);

    // Real stats are set by the subclasses, these are just safe defaults.
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

        // Free the cell the dead unit was standing on. This is the only place it happens:
        // it covers both the target and an attacker killed by counterattack damage.
        AGridManager* Grid = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
        if (Grid)
        {
            Grid->SetCellOccupied(CurrentX, CurrentY, false);
        }

        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);

        // Check for a winner before the actor goes away.
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
        // Ranged attacks ignore obstacles, plain Manhattan distance.
        int32 Distance = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
        bInRange = (Distance <= AttackRange);
    }
    else
    {
        // Melee: the spec says range is computed "like movement". With AttackRange = 1
        // (the only melee value in the game) Manhattan distance equals the shortest path,
        // since the target has to be on an orthogonally adjacent cell.
        int32 ManhattanDistance = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
        bInRange = (ManhattanDistance <= AttackRange);
    }

    if (!bInRange)
    {
        return false;
    }

    // Grab the target's type/distance BEFORE dealing damage:
    // ApplyDamageAmount can destroy the Target, so touching it afterwards isn't safe.
    const int32 DistToTarget = FMath::Abs(CurrentX - Target->CurrentX) + FMath::Abs(CurrentY - Target->CurrentY);
    const bool bTargetIsSniper = Target->IsA(ASniper::StaticClass());
    const bool bTargetIsBrawler = Target->IsA(ABrawler::StaticClass());

    int32 Damage = Attack();
    LastAttackDamage = Damage;
    Target->ApplyDamageAmount(Damage);

    // Counterattack damage: only when the attacker is a Sniper (Ranged) and the target
    // is either a Sniper or a Brawler at distance 1.
    if (AttackType == EAttackType::Ranged)
    {
        if (bTargetIsSniper || (bTargetIsBrawler && (DistToTarget == 1)))
        {
            int32 CounterDamage = FMath::RandRange(1, 3);
            ApplyDamageAmount(CounterDamage);
        }
    }

    // Freeing the target's cell (and the attacker's, if the counterattack kills it)
    // is handled in one place, inside ApplyDamageAmount.
    bHasAttacked = true;
    return true;
}

void ABaseUnit::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);
    // The unit doesn't decide anything itself, it just forwards the click to the game mode.
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

    // Slide towards the current waypoint at constant speed, then move on to the next one.
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
                // Path finished: only now the move counts as done, so the turn can end.
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
        WorldPos.Z += 1.f; // just above the floor to avoid z-fighting
        Waypoints.Add(WorldPos);
    }
    CurrentWaypointIndex = 0;
    bIsMoving = true;
}
