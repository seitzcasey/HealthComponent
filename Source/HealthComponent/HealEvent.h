#pragma once
#include "DamageEvent.h"
#include "HealEvent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct HEALTHCOMPONENT_API FDG_HealEvent : public FDG_DamageEvent
{
    GENERATED_BODY()

    friend class UDG_HealthComponent;
    static const int ClassID = 101;

public:
    FDG_HealEvent() {}

    FDG_HealEvent(double Heal, const UDamageType* DamageType)
        : FDG_DamageEvent(Heal, DamageType)
    {
    
    }

    double GetInitialHeal() const { return GetInitialDamage(); }

    double GetFinalHeal() const { return GetFinalDamage(); }

protected:
    void ModifyHeal(double NewHeal) { ModifyDamage(NewHeal); }
};
