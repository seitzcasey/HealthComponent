#include "DG_HealthComponent.h"
#include "Net/UnrealNetwork.h"

TMulticastDelegate<void(UDG_HealthComponent*, const FDG_DamageEvent&)> UDG_HealthComponent::OnTakeDamage_Static;

TMulticastDelegate<void(UDG_HealthComponent*, const FDG_HealEvent&)> UDG_HealthComponent::OnReceiveHeal_Static;

TMulticastDelegate<void(UDG_HealthComponent*)> UDG_HealthComponent::OnDeath_Static;

TMulticastDelegate<void(UDG_HealthComponent*)> UDG_HealthComponent::OnRevive_Static;

UDG_HealthComponent::UDG_HealthComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    
}

void UDG_HealthComponent::BeginPlay()
{
    Super::BeginPlay();

    DamageLog.Reserve(LogSize);
    HealingLog.Reserve(LogSize);

    if (GetOwner()->HasAuthority())
    {
        // Don't use SetCurrentHealth here as it will trigger Revive.
        CurrentHealth = MaxHealth;

        GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UDG_HealthComponent::HandleOwnerTakeDamage);

        StartHealthRegen();
    }

    if (IsServer())
    {
        BeginReplicatingLogs();
    }
}

void UDG_HealthComponent::ApplyDamage(double Damage)
{
    if (Damage > 0.0)
    {
        SetCurrentHealth(CurrentHealth - Damage);
    }
}

void UDG_HealthComponent::ApplyHeal(double Heal)
{
    if (Heal > 0.0)
    {
        SetCurrentHealth(CurrentHealth + Heal);
    }
}

void UDG_HealthComponent::SetCurrentHealth(double NewHealth)
{
    NewHealth = FMath::Clamp(NewHealth, 0.0, MaxHealth);

    if (CurrentHealth != NewHealth)
    {
        auto PreviousHealth = CurrentHealth;

        CurrentHealth = NewHealth;
        OnHealthChanged.Broadcast(this);

        if (CurrentHealth == 0.0)
        {
            Die();
        }
        else if (PreviousHealth == 0.0 && CurrentHealth > 0.0)
        {
            Revive();
        }
    }
}

void UDG_HealthComponent::SetMaxHealth(double NewMaxHealth)
{
    if (MaxHealth != NewMaxHealth)
    {
        MaxHealth = NewMaxHealth;
        OnHealthChanged.Broadcast(this);
    }
}

void UDG_HealthComponent::SetHealthRegen(double NewHealthRegen)
{
    if (HealthRegen != NewHealthRegen)
    {
        HealthRegen = NewHealthRegen;
        OnHealthChanged.Broadcast(this);
    }
}

void UDG_HealthComponent::SetHealthRegenRate(float NewHealthRegenRate)
{
    if (HealthRegenRate != NewHealthRegenRate)
    {
        HealthRegenRate = NewHealthRegenRate;

        StartHealthRegen();

        OnHealthChanged.Broadcast(this);
    }
}

float UDG_HealthComponent::GetCurrentHealthNormalized() const
{
    check(MaxHealth != 0);
    return CurrentHealth / MaxHealth;
}

double UDG_HealthComponent::GetHealthRegenNormalized() const
{
    const float NormalizedRegenRate = 1.f / HealthRegenRate;
    return HealthRegen * static_cast<double>(NormalizedRegenRate);
}

void UDG_HealthComponent::ClearLogs()
{
    ClearDamageLog();
    ClearHealingLog();
}

void UDG_HealthComponent::ClearDamageLog()
{
    DamageLog.Reset(LogSize);

    if (IsServer())
    {
        bDamageLogDirty = true;
        ReplicateLogs();
    }
    else
    {
        OnDamageLogChanged.Broadcast(this);
    }    
}

void UDG_HealthComponent::ClearHealingLog()
{
    HealingLog.Reset(LogSize);

    if (IsServer())
    {
        bHealingLogDirty = true;
        ReplicateLogs();
    }
    else
    {
        OnHealingLogChanged.Broadcast(this);
    }
}

void UDG_HealthComponent::HandleOwnerTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (DamageType->IsA<UDG_DamageType_Heal>())
    {
        FDG_HealEvent HealEvent = FDG_HealEvent(Damage, DamageType);
        if (HandleReceiveHeal(HealEvent))
        {
            AddActorToHealLog(DamageCauser, HealEvent);
        }
    }
    else
    {
        FDG_DamageEvent DamageEvent = FDG_DamageEvent(Damage, DamageType);
        if (HandleTakeDamage(DamageEvent))
        {
            AddActorToDamageLog(DamageCauser, DamageEvent);
        }
    }
}

bool UDG_HealthComponent::HandleTakeDamage(FDG_DamageEvent& DamageEvent)
{
    ApplyDamageMitigation(DamageEvent);
    return ApplyFinalDamage(DamageEvent);
}

bool UDG_HealthComponent::ApplyDamageMitigation(FDG_DamageEvent& DamageEvent)
{
    if (false /*We only broadcast if modified*/)
    {
        DamageEvent.ModifyDamage(DamageEvent.GetInitialDamage());
        OnDamageMitigated.Broadcast(this, DamageEvent);
        return true;
    }

    return false;
}

bool UDG_HealthComponent::ApplyFinalDamage(FDG_DamageEvent& DamageEvent)
{
    const double FinalDamage = DamageEvent.GetFinalDamage();
    if (FinalDamage > 0.0)
    {
        ApplyDamage(FinalDamage);
        OnTakeDamage.Broadcast(this, DamageEvent);
        OnTakeDamage_Static.Broadcast(this, DamageEvent);
        return true;
    }
    return false;
}

bool UDG_HealthComponent::HandleReceiveHeal(FDG_HealEvent& HealEvent)
{
    ApplyHealAmplification(HealEvent);
    return ApplyFinalHeal(HealEvent);
}

bool UDG_HealthComponent::ApplyHealAmplification(FDG_HealEvent& HealEvent)
{
    if (false /*We only broadcast if modified*/)
    {
        HealEvent.ModifyHeal(HealEvent.GetInitialHeal());
        OnHealAmplified.Broadcast(this, HealEvent);
        return true;
    }
    return false;
}

bool UDG_HealthComponent::ApplyFinalHeal(FDG_HealEvent& HealEvent)
{
    const double FinalHeal = HealEvent.GetFinalHeal();
    if (FinalHeal > 0.0)
    {
        ApplyHeal(FinalHeal);
        OnReceiveHeal.Broadcast(this, HealEvent);
        OnReceiveHeal_Static.Broadcast(this, HealEvent);
        return true;
    }
    return false;
}

void UDG_HealthComponent::Die()
{
    if (IsDead())
    {
        return;
    }

    StopHealthRegen();

    OnDeath.Broadcast(this);
    OnDeath_Static.Broadcast(this);
}

void UDG_HealthComponent::Revive()
{
    if (!IsDead())
    {
        return;
    }

    StartHealthRegen();

    OnRevive.Broadcast(this);
    OnRevive_Static.Broadcast(this);
}

void UDG_HealthComponent::StartHealthRegen()
{
    if (HealthRegenRate > 0.f && HealthRegen > 0.0)
    {
        FTimerManager& TimerManager = GetWorld()->GetTimerManager();
        if (TimerManager.IsTimerActive(TimerHandle_HealthRegen))
        {
            return;
        }

        TimerManager.SetTimer(TimerHandle_HealthRegen, this, &UDG_HealthComponent::HandleHealthRegen, HealthRegenRate, true);
        OnStartHealthRegen.Broadcast(this);

        if (bRegenImmediately)
        {
            HandleHealthRegen();
        }
    }
}

void UDG_HealthComponent::HandleHealthRegen()
{
    if (HealthRegen > 0.0)
    {
        if (bRegenIsHealing)
        {
            ApplyHeal(HealthRegen);
        }
        else
        {
            SetCurrentHealth(CurrentHealth + HealthRegen);
        }

        OnHealthRegen.Broadcast(this, HealthRegen);
    }
}

void UDG_HealthComponent::StopHealthRegen()
{
    FTimerManager& TimerManager = GetWorld()->GetTimerManager();
    if (TimerManager.IsTimerActive(TimerHandle_HealthRegen))
    {
        TimerManager.ClearTimer(TimerHandle_HealthRegen);
        OnStopHealthRegen.Broadcast(this);
    }
}

void UDG_HealthComponent::AddActorToDamageLog(AActor* Actor, const FDG_DamageEvent& DamageEvent)
{
    const int32 Index = DamageLog.IndexOfByKey(FDG_HealthSystemLogItem(Actor));

    if (Index != INDEX_NONE)
    {
        auto Item = DamageLog[Index];
        Item.Amount += DamageEvent.GetFinalDamage();
        DamageLog.RemoveAt(Index, 1, false);
        DamageLog.Insert(Item, 0);
    }
    else
    {
        if (DamageLog.Num() == LogSize)
        {
            DamageLog.RemoveAt(LogSize - 1, 1, false);
        }

        DamageLog.Insert(FDG_HealthSystemLogItem(Actor, Actor ? GetActorName(Actor) : "Unknown", DamageEvent.GetFinalDamage()), 0);
    }
    
    if (IsServer())
    {
        bDamageLogDirty = true;
    }
    else
    {
        OnDamageLogChanged.Broadcast(this);
    }
}

void UDG_HealthComponent::AddActorToHealLog(AActor* Actor, const FDG_HealEvent& HealEvent)
{
    const int32 Index = HealingLog.IndexOfByKey(Actor);

    if (Index != INDEX_NONE)
    {
        auto Item = HealingLog[Index];
        Item.Amount += HealEvent.GetFinalHeal();
        HealingLog.RemoveAt(Index, 1, false);
        HealingLog.Insert(Item, 0);
    }
    else
    {
        if (HealingLog.Num() == LogSize)
        {
            HealingLog.RemoveAt(LogSize - 1, 1, false);
        }

        HealingLog.Insert(FDG_HealthSystemLogItem(Actor, Actor ? GetActorName(Actor) : "Unknown", HealEvent.GetFinalHeal()), 0);
    }

    if (IsServer())
    {
        bHealingLogDirty = true;
    }
    else
    {
        OnHealingLogChanged.Broadcast(this);
    }
}

FString UDG_HealthComponent::GetActorName(AActor* Actor) const
{
    if (Actor)
    {
        return Actor->GetName();
    }

    return FString();
}

void UDG_HealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UDG_HealthComponent, ReplicatedDamageLog, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDG_HealthComponent, ReplicatedHealingLog, COND_OwnerOnly);
}

bool UDG_HealthComponent::IsServer() const
{
    return GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer;
}

void UDG_HealthComponent::BeginReplicatingLogs()
{
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ReplicateLogs, this, &UDG_HealthComponent::ReplicateLogs, LogReplicationRate, true);
}

void UDG_HealthComponent::ReplicateLogs()
{
    if (bDamageLogDirty)
    {
        ReplicatedDamageLog = DamageLog;
        bDamageLogDirty = false;
    }

    if (bHealingLogDirty)
    {
        ReplicatedHealingLog = HealingLog;
        bHealingLogDirty = false;
    }
}

void UDG_HealthComponent::OnRep_DamageLog()
{
    DamageLog = ReplicatedDamageLog;
    OnDamageLogChanged.Broadcast(this);
}

void UDG_HealthComponent::OnRep_HealingLog()
{
    HealingLog = ReplicatedHealingLog;
    OnHealingLogChanged.Broadcast(this);
}
