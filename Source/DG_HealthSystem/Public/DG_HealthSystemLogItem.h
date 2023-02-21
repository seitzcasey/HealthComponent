#pragma once
#include "DG_HealthSystemLogItem.generated.h"


USTRUCT()
struct FDG_HealthSystemLogItem
{
    GENERATED_BODY()

    // Actor and ActorName are stored because the actor could have been destroyed.

    UPROPERTY()
    TWeakObjectPtr<AActor> Actor;

    UPROPERTY()
    FString ActorName;

    UPROPERTY()
    double Amount;

    FDG_HealthSystemLogItem() {}

    // helper for implicit TArray::Find
    FDG_HealthSystemLogItem(AActor* InActor)
        : Actor(InActor)
        , ActorName("Unknown")
        , Amount(0.0)
    {
    
    }

    FDG_HealthSystemLogItem(AActor* InActor, const FString& InActorName, double InAmount)
        : Actor(InActor)
        , ActorName(InActorName)
        , Amount(InAmount)
    {
    
    }

    // We only consider items with the same actor the same because some actors
    // could have the same name even though they are different.
    bool operator==(const FDG_HealthSystemLogItem& Other) const
    {
        return Actor.IsValid() && Actor == Other.Actor;
    }

    bool operator!=(const FDG_HealthSystemLogItem& Other) const
    {
        return !(*this == Other);
    }

    bool operator==(FDG_HealthSystemLogItem& Other)
    {
        return Actor.IsValid() && Actor == Other.Actor;
    }

    bool operator!=(FDG_HealthSystemLogItem& Other)
    {
        return !(*this == Other);
    }
};
