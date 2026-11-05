#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Melee   UMETA(DisplayName = "Melee"),
    Ranged  UMETA(DisplayName = "Ranged")
};

UCLASS(Abstract)
class PROGETTOPAA_API ABaseUnit : public AActor
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 CurrentX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 CurrentY;

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

    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual int32 Attack();

    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual void ApplyDamageAmount(int32 DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual bool IsAlive() const;

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

    void StartSmoothMovement(const TArray<FIntPoint>& Path, AGridManager* GridManager);

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MaxHealth;

    // Danno realmente inflitto nell'ultimo PerformAttack (usato per lo storico mosse).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 LastAttackDamage;
};
