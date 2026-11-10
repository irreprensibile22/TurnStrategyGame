#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.generated.h"

// Melee units hit adjacent cells only, ranged units can shoot from a distance.
UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Melee   UMETA(DisplayName = "Melee"),
    Ranged  UMETA(DisplayName = "Ranged")
};

// Common base for every unit on the board. Subclasses (Sniper, Brawler) only
// set their stats in the constructor, all the gameplay logic lives here.
UCLASS(Abstract)
class TURNSTRATEGY_API ABaseUnit : public AActor
{
    GENERATED_BODY()

public:
    ABaseUnit();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    UStaticMeshComponent* VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MaxMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    EAttackType AttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MinDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MaxDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 Health;

    // Position on the logical grid (cell indices, not world coordinates).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 CurrentX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 CurrentY;

    // Per-turn action flags, reset at the start of every turn.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    bool bHasMoved;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    bool bHasAttacked;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    bool bIsSelected;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Unit")
    bool bIsPlayerUnit;

    UFUNCTION(BlueprintCallable, Category = "Unit")
    void SetSelected(bool bSelected);

    // Rolls the damage only, doesn't touch the target. See PerformAttack for the full attack.
    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual int32 Attack();

    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual void ApplyDamageAmount(int32 DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual bool IsAlive() const;

    // Range check + damage + counterattack. Returns false if the attack wasn't allowed.
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool PerformAttack(ABaseUnit* Target, class AGridManager* GridManager);

    virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

    UFUNCTION(BlueprintCallable, Category = "Unit")
    void SetOpponentMaterial(UMaterialInterface* NewMaterial);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed; 

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    TArray<FVector> Waypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    int32 CurrentWaypointIndex;

    // Turns a grid path into world waypoints; Tick then walks the unit along them.
    void StartSmoothMovement(const TArray<FIntPoint>& Path, AGridManager* GridManager);

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MaxHealth;

    // Damage actually dealt by the last PerformAttack (used by the move history).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 LastAttackDamage;
};
