#include "Brawler.h"
#include "UObject/ConstructorHelpers.h"

// Close-range fighter: moves far, hits adjacent cells only, lots of health.
ABrawler::ABrawler()
{
    MaxMovement = 6;
    AttackType = EAttackType::Melee;
    AttackRange = 1;
    MinDamage = 1;
    MaxDamage = 6;
    Health = 40;
    MaxHealth = 40;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(TEXT("Material'/Game/texture/Soldier2_Bluee.Soldier2_Bluee'"));
    if (DefaultMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, DefaultMaterial.Object);
    }
}
