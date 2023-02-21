#pragma once
#include "Components/ActorComponent.h"
#include "DG_HealthSystemLogItem.h"
#include "DG_HealthComponent.generated.h"

struct FDG_DamageEvent;
struct FDG_HealEvent;
class AActor;
class UDamageType;
class AController;

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class DG_HEALTHSYSTEM_API UDG_HealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDG_HealthComponent(const FObjectInitializer& ObjectInitializer);

    // Begin ActorComponent Interface
    virtual void BeginPlay() override;
    // End ActorComponent Interface

    // Begin State
    UFUNCTION(BlueprintCallable)
    void ApplyDamage(double Damage);

    UFUNCTION(BlueprintCallable)
    void ApplyHeal(double Heal);

    UFUNCTION(BlueprintCallable)
    bool IsDead() const { return CurrentHealth == 0.0; }
    // End State

    // Begin Setters
    UFUNCTION(BlueprintCallable)
    void SetCurrentHealth(double NewHealth);

    UFUNCTION(BlueprintCallable)
    void SetMaxHealth(double NewMaxHealth);

    UFUNCTION(BlueprintCallable)
    void SetHealthRegen(double NewHealthRegen);

    UFUNCTION(BlueprintCallable)
    void SetHealthRegenRate(float NewHealthRegenRate);
    // End Setters

    // Begin Getters
    UFUNCTION(BlueprintCallable)
    double GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintCallable)
    double GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintCallable)
    float GetCurrentHealthNormalized() const;

    UFUNCTION(BlueprintCallable)
    double GetHealthRegen() const { return HealthRegen; }

    UFUNCTION(BlueprintCallable)
    float GetHealthRegenRate() const { return HealthRegenRate; }

    // Return HealthRegen in terms of 1 second regardless of regen rate.
    UFUNCTION(BlueprintCallable)
    double GetHealthRegenNormalized() const;

    const TArray<FDG_HealthSystemLogItem>& GetDamageLog() const { return DamageLog; }

    TArray<FDG_HealthSystemLogItem> GetDamageLog() { return DamageLog; }

    const TArray<FDG_HealthSystemLogItem>& GetHealingLog() const { return DamageLog; }

    TArray<FDG_HealthSystemLogItem> GetHealingLog() { return HealingLog; }
    // End Getters

    // Begin Logging
    void ClearLogs();

    void ClearDamageLog();

    void ClearHealingLog();
    // End Logging

protected:
    // Begin Main Logic
    // Callback for Owner's OnTakeAnyDamage delegate.
    UFUNCTION()
    virtual void HandleOwnerTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    // Begin Damage Logic
    virtual bool HandleTakeDamage(FDG_DamageEvent& DamageEvent);

    // Return true if any damage was mitigated.
    virtual bool ApplyDamageMitigation(FDG_DamageEvent& DamageEvent);

    // Applies the final damage from the event.
    virtual bool ApplyFinalDamage(FDG_DamageEvent& DamageEvent);
    // End Damage Logic

    // Begin Healing Logic
    virtual bool HandleReceiveHeal(FDG_HealEvent& HealEvent);
    
    virtual bool ApplyHealAmplification(FDG_HealEvent& HealEvent);

    virtual bool ApplyFinalHeal(FDG_HealEvent& HealEvent);
    // End Healing Logic

    virtual void Die();
    
    virtual void Revive();
    // End Main Logic

    // Begin Regen Logic
    virtual void StartHealthRegen();
    
    virtual void HandleHealthRegen();

    virtual void StopHealthRegen();
    // End Regen Logic

    // Begin Logging Logic
    virtual void AddActorToDamageLog(AActor* Actor, const FDG_DamageEvent& DamageEvent);
    
    virtual void AddActorToHealLog(AActor* Actor, const FDG_HealEvent& HealEvent);

    // This is a helper function that can be used for adding names to the damage log.
    // This is what will show in that damage log.
    virtual FString GetActorName(AActor* Actor) const; 
    // End Logging Logic

    // Begin Replication Logic
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool IsServer() const;

    void BeginReplicatingLogs();

    void ReplicateLogs();

    // Begin RepNotifies
    UFUNCTION()
    void OnRep_DamageLog();

    UFUNCTION()
    void OnRep_HealingLog();
    // End RepNotifies
    // End Replication Logic

public:
    // static delegate to announce all damage taken. This is called when ApplyFinalDamage is called.
    // could be useful for global damage tracking for achievements and such.
    static TMulticastDelegate<void(UDG_HealthComponent*, const FDG_DamageEvent&)> OnTakeDamage_Static;

    // static delegate to announce all healing received.
    static TMulticastDelegate<void(UDG_HealthComponent*, const FDG_HealEvent&)> OnReceiveHeal_Static;

    // static delegate to announce all deaths.
    static TMulticastDelegate<void(UDG_HealthComponent*)> OnDeath_Static;

    // static delegate to announce all revives.
    static TMulticastDelegate<void(UDG_HealthComponent*)> OnRevive_Static;

    // Same as above but per instance
    TMulticastDelegate<void(UDG_HealthComponent*, const FDG_DamageEvent&)> OnTakeDamage;

    // Called when damage was mitigated
    // ie: Absorb, resist, etc
    TMulticastDelegate<void(UDG_HealthComponent*, const FDG_DamageEvent&)> OnDamageMitigated;

    // Called when healing is received
    TMulticastDelegate<void(UDG_HealthComponent*, const FDG_HealEvent&)> OnReceiveHeal;

    // Called when healing was amplified
    TMulticastDelegate<void(UDG_HealthComponent*, const FDG_HealEvent&)> OnHealAmplified;

    // Called when any type of health related property changes
    // ie: CurrentHealth, MaxHealth, Regen, RegenRate, etc
    TMulticastDelegate<void(UDG_HealthComponent*)> OnHealthChanged;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnDeath;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnRevive;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnStartHealthRegen;

    TMulticastDelegate<void(UDG_HealthComponent*, double)> OnHealthRegen;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnStopHealthRegen;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnDamageLogChanged;

    TMulticastDelegate<void(UDG_HealthComponent*)> OnHealingLogChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double MaxHealth = 100.0;

    // Amount of health to increase CurrentHealth by each "HealthRegenRate" interval
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double HealthRegen;

    // Interval to apply HealthRegen
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HealthRegenRate;

    // This controls whether health regen triggers healing received events.
    UPROPERTY(EditInstanceOnly)
    bool bRegenIsHealing = false;

    // This controls whether the health regen should happen when starting health regen or wait 1 cycle.
    UPROPERTY(EditInstanceOnly)
    bool bRegenImmediately = false;

    UPROPERTY(EditInstanceOnly, Category="Logging")
    bool bLoggingEnabled = false;

    UPROPERTY(EditInstanceOnly, Category="Logging")
    int32 LogSize = 10;

    UPROPERTY(EditInstanceOnly, Category="Logging")
    float LogReplicationRate = 0.5;

    // Logging
    TArray<FDG_HealthSystemLogItem> DamageLog;

    TArray<FDG_HealthSystemLogItem> HealingLog;

    // The replication logic is separated because these arrays can change a lot between each frame
    UPROPERTY(ReplicatedUsing=OnRep_DamageLog)
    TArray<FDG_HealthSystemLogItem> ReplicatedDamageLog;

    // The replication logic is separated because these arrays can change a lot between each frame
    UPROPERTY(ReplicatedUsing = OnRep_HealingLog)
    TArray<FDG_HealthSystemLogItem> ReplicatedHealingLog;

    // This is only used in multiplayer.
    bool bDamageLogDirty = false;

    // This is only used in multiplayer.
    bool bHealingLogDirty = false;

    // Begin Timer Handles
    FTimerHandle TimerHandle_HealthRegen;

    FTimerHandle TimerHandle_ReplicateLogs;
    // End Timer Handles
};
