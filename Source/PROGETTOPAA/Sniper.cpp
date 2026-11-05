#include "Sniper.h"
#include "UObject/ConstructorHelpers.h"

ASniper::ASniper()
{
    MaxMovement = 3;
    AttackType = EAttackType::Ranged;
    AttackRange = 10;
    MinDamage = 4;
    MaxDamage = 8;
    Health = 20;
    MaxHealth = 20;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(TEXT("Material'/Game/texture/Soldier1_Bluee.Soldier1_Bluee'"));
    if (DefaultMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, DefaultMaterial.Object);
    }
}
