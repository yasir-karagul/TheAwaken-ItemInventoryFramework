#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "InventoryServices.generated.h"

class APlayerState;

UCLASS()
class AWAKEN_API UInventoryServices : public UGameInstanceSubsystem{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static UInventoryServices* get(const UObject* worldContext);

    // Real frameworks call this from their own initialization to replace a stub.
    UFUNCTION(BlueprintCallable, Category = "Awaken|Inventory")
    bool registerSeam(UClass* interfaceClass, UObject* implementation);

    UObject* getSeam(const UClass* interfaceClass) const;

    // ---- facade (Command / Query as labelled) ----
    int64 getGameTimeSeconds() const;                                              // Query
    bool isIncapacitated(APlayerState* player) const;                              // Query
    bool isCarryActive(APlayerState* player) const;                                // Query
    bool isInInteractRange(APlayerState* player, AActor* target) const;            // Query
    bool isMeterActive(const FGameplayTag& meterTag) const;                        // Query
    void modifyMeter(APlayerState* player, const FGameplayTag& meterTag, float delta) const;            // Command
    void registerIllnessSource(APlayerState* player, const FGameplayTag& sourceTag, float hours) const; // Command
    void applyIllnessDamage(APlayerState* player, float amount) const;             // Command
    void restoreHealth(APlayerState* player, float amount) const;                  // Command
    bool hasFreeCarrySlot(APlayerState* player, int32 carryCost) const;            // Query
    bool beginCarry(APlayerState* player, FName itemId) const;                     // Command
    FS_GearReleaseResult releaseEquippedGear(APlayerState* player) const;          // Command
    bool reEquipGear(APlayerState* player, const TArray<FS_GearSlotRecord>& gear) const; // Command
    AActor* spawnLooseItem(UObject* worldContext, const FS_InventoryEntry& entry, FVector location) const; // Command
    UObject* getBagService() const;

    // ---- seeded random stream (S8.8): chance outcomes repeat in tests (Doc B22, Index A109 S7-F19) ----
    void setRandomSeed(int32 seed);                                                // Command
    float rollChance();                                                            // Command (advances the stream)

private:
    UPROPERTY() TMap<TObjectPtr<UClass>, TObjectPtr<UObject>> seams;
    FRandomStream randomStream;
};
