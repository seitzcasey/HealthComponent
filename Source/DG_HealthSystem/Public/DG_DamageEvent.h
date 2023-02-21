#pragma once
#include "Engine/DamageEvents.h"
#include "DG_DamageEvent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct DG_HEALTHSYSTEM_API FDG_DamageEvent : public FDamageEvent
{
    GENERATED_BODY()

    friend class UDG_HealthComponent;
    static const int ClassID = 100;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double InitialDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double FinalDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDamageModified = false;

public:
    FDG_DamageEvent() {}

    FDG_DamageEvent(double Damage, const UDamageType* DamageType)
        : InitialDamage(Damage)
        , FinalDamage(InitialDamage)
        , bDamageModified(false)
    {
        if (DamageType)
        {
            DamageTypeClass = DamageType->GetClass();
        }
    }

    double GetInitialDamage() const { return InitialDamage; }

    double GetFinalDamage() const { return FinalDamage; }

protected:
    virtual void ModifyDamage(double NewDamage)
    {
        if (bDamageModified)
        {
            return;
        }

        FinalDamage = NewDamage;
        bDamageModified = true;
    }
};
