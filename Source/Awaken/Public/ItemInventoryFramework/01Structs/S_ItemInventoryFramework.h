#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "S_ItemInventoryFramework.generated.h"

class UInventoryData;
class APlayerState;

// ---------- Item state (R01 §3.2) ----------

USTRUCT(BlueprintType)
struct FS_ItemInstanceState{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float durability = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid containedInventoryId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool isLoaded = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 loadedRounds = 0;
};

// ---------- One inventory entry (R01 §4) ----------
// Stackable: itemId + totalQuantity + reservedQuantity (+ spoilAtTime when the item can spoil).
// Instanced: itemId + instanceId + state, totalQuantity is always 1.
// spoilAtTime is an absolute game time in game seconds (Doc02 §19.4); -1 means "does not spoil".

USTRUCT(BlueprintType)
struct FS_InventoryEntry : public FFastArraySerializerItem{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName itemId;
    UPROPERTY(BlueprintReadOnly) int32 totalQuantity = 0;
    UPROPERTY(BlueprintReadOnly) int32 reservedQuantity = 0;
    UPROPERTY(BlueprintReadOnly) int64 spoilAtTime = -1;
    UPROPERTY(BlueprintReadOnly) FGuid instanceId;
    UPROPERTY(BlueprintReadOnly) FS_ItemInstanceState state;

    bool isInstanced() const{
        return instanceId.IsValid();
    }

    int32 availableQuantity() const{
        return totalQuantity - reservedQuantity;
    }
};

// Fast array wrapper (R01 §3.3). The owner pointer is not replicated; it is set in UInventoryData.
USTRUCT()
struct FS_InventoryEntryArray : public FFastArraySerializer{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FS_InventoryEntry> items;

    UPROPERTY(NotReplicated)
    TObjectPtr<UInventoryData> owner = nullptr;

    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& parameters);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParms){
        return FFastArraySerializer::FastArrayDeltaSerialize<FS_InventoryEntry, FS_InventoryEntryArray>(items, deltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FS_InventoryEntryArray> : public TStructOpsTypeTraitsBase2<FS_InventoryEntryArray>{
    enum{
        WithNetDeltaSerializer = true
    };
};

// ---------- Requests and results (Doc02 §6: explained rejection) ----------

USTRUCT(BlueprintType)
struct FS_ItemStack{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName itemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 quantity = 0;
};

// success == the failReason tag is empty. Never a bare bool.
USTRUCT(BlueprintType)
struct FS_InventoryResult{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGameplayTag failReason;
    UPROPERTY(BlueprintReadOnly) int32 quantity = 0;

    bool succeeded() const{
        return !failReason.IsValid();
    }
};

// R01 §3.7.
USTRUCT(BlueprintType)
struct FS_GrantResult{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 quantityStored = 0;
    UPROPERTY(BlueprintReadOnly) int32 quantityCarried = 0;
    UPROPERTY(BlueprintReadOnly) int32 quantityDropped = 0;
    UPROPERTY(BlueprintReadOnly) FGameplayTag failReason;
};

USTRUCT(BlueprintType)
struct FS_HotbarBinding{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 slotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName boundItemId;
};

// R01 §3.6. sessionId is the spec's "StationID"; a hand craft has no station, so it is a GUID.
USTRUCT(BlueprintType)
struct FS_StagedReservation{
    GENERATED_BODY()

    UPROPERTY() FName itemId;
    UPROPERTY() FGuid instanceId;
    UPROPERTY() int32 reservedQuantity = 0;
    UPROPERTY() FGuid sessionId;
};

// R01 §3.1 / §3.2a. One list covers meter changes, the illness chance and the HP loss.
//  MeterChange   : tag = meter tag, amount = signed delta
//  IllnessChance : tag = illness source tag, amount = chance 0..1, sourceDurationHours = SourceDuration
//  HealthLoss    : amount = HP lost, applied every time (R01 §3.2a, Index A96 q139)
USTRUCT(BlueprintType)
struct FS_ConsumeEffect{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) E_ConsumeEffectKind kind = E_ConsumeEffectKind::MeterChange;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag tag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float amount = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float sourceDurationHours = 0.0f;
};

// ---------- Bag / gear / held item (R01 §3.5, §3.8) ----------

USTRUCT(BlueprintType)
struct FS_GearSlotRecord{
    GENERATED_BODY()

    UPROPERTY() FGuid instanceId;
    UPROPERTY() int32 stackIndex = 0;
};

// The R01-owned part of R13's S_BagRecord (R01 §4, R13 §3.6).
// No worn clothes, no hold-slot items, no reservations (R13 §3.6, Index A110 XQ-S2-02).
USTRUCT(BlueprintType)
struct FS_BagContents{
    GENERATED_BODY()

    UPROPERTY() FGuid personalInventoryId;
    UPROPERTY() TArray<FS_HotbarBinding> hotbarSnapshot;
    UPROPERTY() TArray<FS_GearSlotRecord> gear;
    UPROPERTY() FGuid wieldedInstanceId;   // a wielded weapon is listed once, pointing at the same instance (Index A110 Q-S2-01)
};

USTRUCT(BlueprintType)
struct FS_BagDetachResult{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGameplayTag failReason;
    UPROPERTY(BlueprintReadOnly) FGuid bagId;

    bool succeeded() const{
        return !failReason.IsValid();
    }
};

USTRUCT(BlueprintType)
struct FS_GearReleaseResult{
    GENERATED_BODY()

    UPROPERTY() bool succeeded = true;
    UPROPERTY() TArray<FS_InventoryEntry> gearEntries;
    UPROPERTY() TArray<FS_GearSlotRecord> gearSlots;
};

USTRUCT(BlueprintType)
struct FS_HeldItemView{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName itemId;
    UPROPERTY(BlueprintReadOnly) FGuid instanceId;
    UPROPERTY(BlueprintReadOnly) bool isUnarmed = true;
};

USTRUCT(BlueprintType)
struct FS_HotbarResolve{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool valid = false;
    UPROPERTY(BlueprintReadOnly) FName itemId;
    UPROPERTY(BlueprintReadOnly) FGuid instanceId;
    UPROPERTY(BlueprintReadOnly) int32 availableQuantity = 0;
};

// Replicated owner-only so the hover card can mark "next to use" (R01 §3.1a display only).
USTRUCT(BlueprintType)
struct FS_LastUsedRecord{
    GENERATED_BODY()

    UPROPERTY() FName itemId;
    UPROPERTY() FGuid instanceId;
};

USTRUCT(BlueprintType)
struct FS_SpoilResult{
    GENERATED_BODY()

    UPROPERTY() bool converted = false;
    UPROPERTY() int32 releasedReservedQuantity = 0;
    UPROPERTY() FName fromItemId;
    UPROPERTY() FName toItemId;
};

// ---------- Content layout data (R01 §3.1a) ----------

USTRUCT(BlueprintType)
struct FS_InventorySlotDef{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName slotId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D position = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D size = FVector2D(1.0, 1.0);
};

USTRUCT(BlueprintType)
struct FS_StackUnitPlacement{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator rotation = FRotator::ZeroRotator;
};

// What the bag screen draws for one slot (R01 §3.1b).
USTRUCT(BlueprintType)
struct FS_SlotView{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName slotId;
    UPROPERTY(BlueprintReadOnly) FName itemId;
    UPROPERTY(BlueprintReadOnly) int32 quantity = 0;
    UPROPERTY(BlueprintReadOnly) bool isVariantPair = false;
    UPROPERTY(BlueprintReadOnly) FName variantItemId;
    UPROPERTY(BlueprintReadOnly) int32 variantQuantity = 0;
};

USTRUCT(BlueprintType)
struct FS_InstanceCopyView{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid instanceId;
    UPROPERTY(BlueprintReadOnly) float durability = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool isNextForHotbar = false;
    UPROPERTY(BlueprintReadOnly) bool isNextForCrafting = false;
};

// ---------- Save records (R01 §8, R13 §3.7) ----------

USTRUCT()
struct FS_SavedInventoryEntry{
    GENERATED_BODY()

    UPROPERTY() FName itemId;
    UPROPERTY() int32 totalQuantity = 0;
    UPROPERTY() FGuid instanceId;
    UPROPERTY() FS_ItemInstanceState state;
    UPROPERTY() int64 spoilValue = -1;   // D3: absolute in World files, time left in Character files
};

USTRUCT()
struct FS_InventorySaveRecord{
    GENERATED_BODY()

    UPROPERTY() FGuid inventoryId;
    UPROPERTY() E_InventoryType type = E_InventoryType::Personal;
    UPROPERTY() bool isDeathBag = false;
    UPROPERTY() int32 containerLimit = 0;
    UPROPERTY() TArray<FGameplayTag> allowedTags;
    UPROPERTY() TArray<FS_SavedInventoryEntry> entries;
};

// First record = the inventory itself; the rest = nested Portable Container contents (saved once, with the holder; R01 §3.4).
USTRUCT()
struct FS_InventorySaveBundle{
    GENERATED_BODY()

    UPROPERTY() TArray<FS_InventorySaveRecord> records;
};

USTRUCT()
struct FS_HotbarSaveRecord{
    GENERATED_BODY()

    UPROPERTY() int32 version = 1;
    UPROPERTY() TArray<FS_HotbarBinding> bindings;
};

USTRUCT()
struct FS_CharacterInventorySave{
    GENERATED_BODY()

    UPROPERTY() int32 version = 1;
    UPROPERTY() FS_InventorySaveBundle personal;
    UPROPERTY() FS_HotbarSaveRecord hotbar;
};

USTRUCT()
struct FS_WorldInventorySave{
    GENERATED_BODY()

    UPROPERTY() int32 version = 1;
    UPROPERTY() TArray<FS_InventorySaveBundle> inventories;
};

// ---------- Message payloads (R01 §6, Doc02 §11) ----------

USTRUCT(BlueprintType)
struct FS_InventoryChangedMessage{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid inventoryId;
};

USTRUCT(BlueprintType)
struct FS_GrantDroppedMessage{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) TObjectPtr<APlayerState> player = nullptr;
    UPROPERTY(BlueprintReadOnly) FName itemId;
    UPROPERTY(BlueprintReadOnly) int32 quantity = 0;
};

USTRUCT(BlueprintType)
struct FS_HotbarChangedMessage{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) TObjectPtr<APlayerState> player = nullptr;
    UPROPERTY(BlueprintReadOnly) int32 slotIndex = 0;
};
