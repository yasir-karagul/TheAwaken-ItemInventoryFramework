// TEMPORARY STUB — owner: see each class below. Delete each class when the real framework lands.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInventoryFramework/03Interfaces/BPI_GameClock.h"
#include "ItemInventoryFramework/03Interfaces/BPI_SurvivalSink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_VitalsSink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_PlayerStateQuery.h"
#include "ItemInventoryFramework/03Interfaces/BPI_CarrySink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_EquipmentGear.h"
#include "ItemInventoryFramework/03Interfaces/BPI_LooseItemSpawner.h"
#include "ItemInventoryFramework/03Interfaces/BPI_BagActorService.h"
#include "InventoryStubs.generated.h"

// TEMPORARY STUB — owner: Doc R12 Time. A settable clock for tests and early play.
UCLASS()
class UStubGameClock : public UObject, public IBPI_GameClock{
    GENERATED_BODY()

public:
    UPROPERTY() int64 gameSeconds = 0;

    virtual int64 getGameTimeSeconds_Implementation() override;
};

// TEMPORARY STUB — owners: Doc B03 / Doc B13. Logs; every meter active unless switched off.
UCLASS()
class UStubSurvivalSink : public UObject, public IBPI_SurvivalSink{
    GENERATED_BODY()

public:
    UPROPERTY() bool illnessActive = true;

    virtual void modifyMeter_Implementation(APlayerState* player, FGameplayTag meterTag, float delta) override;
    virtual void registerIllnessSource_Implementation(APlayerState* player, FGameplayTag sourceTag, float durationHours) override;
    virtual bool isMeterActive_Implementation(FGameplayTag meterTag) override;
};

// TEMPORARY STUB — owners: Doc R03 / Doc R08. Logs the call.
UCLASS()
class UStubVitalsSink : public UObject, public IBPI_VitalsSink{
    GENERATED_BODY()

public:
    virtual void applyIllnessDamage_Implementation(APlayerState* player, float amount) override;
    virtual void restoreHealth_Implementation(APlayerState* player, float amount) override;
};

// TEMPORARY STUB — owners: Doc B02 / B07 / B18. Everything is allowed by default; tests flip the flags.
UCLASS()
class UStubPlayerStateQuery : public UObject, public IBPI_PlayerStateQuery{
    GENERATED_BODY()

public:
    UPROPERTY() bool incapacitated = false;
    UPROPERTY() bool carryActive = false;

    virtual bool isIncapacitated_Implementation(APlayerState* player) override;
    virtual bool isCarryActive_Implementation(APlayerState* player) override;
    virtual bool isInInteractRange_Implementation(APlayerState* player, AActor* target) override;
};

// TEMPORARY STUB — owner: Doc B07. Answers "no free slot", so heavy items land loose.
UCLASS()
class UStubCarrySink : public UObject, public IBPI_CarrySink{
    GENERATED_BODY()

public:
    UPROPERTY() bool slotFree = false;

    virtual bool hasFreeCarrySlot_Implementation(APlayerState* player, int32 carryCost) override;
    virtual bool beginCarry_Implementation(APlayerState* player, FName itemId) override;
};

// TEMPORARY STUB — owner: Doc R05. Releases nothing.
UCLASS()
class UStubEquipmentGear : public UObject, public IBPI_EquipmentGear{
    GENERATED_BODY()

public:
    UPROPERTY() bool failRelease = false;   // test hook for the atomicity test (Section 9)

    virtual FS_GearReleaseResult releaseEquippedGear_Implementation(APlayerState* player) override;
    virtual bool reEquipGear_Implementation(APlayerState* player, const TArray<FS_GearSlotRecord>& gear) override;
};

// TEMPORARY STUB — owner: Doc02 §19.7 loose-body owner (D4). Spawns ALooseItemActor (Section 5).
UCLASS()
class UStubLooseItemSpawner : public UObject, public IBPI_LooseItemSpawner{
    GENERATED_BODY()

public:
    virtual AActor* spawnLooseItem_Implementation(UObject* worldContext, const FS_InventoryEntry& entry, FVector location) override;
};

// TEMPORARY STUB — owners: Doc B02 / Doc R13. Keeps bag records in memory only.
UCLASS()
class UStubBagActorService : public UObject, public IBPI_BagActorService{
    GENERATED_BODY()

public:
    UPROPERTY() bool failCreate = false;   // test hook

    virtual FGuid createBagRecordAndActor_Implementation(APlayerState* player, FVector location, FGuid characterId, FGuid inventoryId) override;
    virtual bool deleteBag_Implementation(FGuid bagId) override;
    virtual void finalizeBag_Implementation(FGuid bagId, const FS_BagContents& contents) override;
    virtual bool bagExists_Implementation(FGuid bagId) override;
    virtual FS_BagContents getBagContents_Implementation(FGuid bagId) override;
    virtual FGuid getBagOwner_Implementation(FGuid bagId) override;
    virtual FVector getBagLocation_Implementation(FGuid bagId) override;
    virtual void removeBagsOwnedBy_Implementation(FGuid characterId) override;

private:
    UPROPERTY() TMap<FGuid, FGuid> bagOwners;
    UPROPERTY() TMap<FGuid, FVector> bagLocations;
    UPROPERTY() TMap<FGuid, FS_BagContents> bagContents;
};
