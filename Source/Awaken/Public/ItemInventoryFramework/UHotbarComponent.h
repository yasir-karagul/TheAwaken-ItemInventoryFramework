#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UHotbarComponent.generated.h"

class APlayerState;
class UInventoryData;

UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UHotbarComponent : public UActorComponent{
    GENERATED_BODY()

public:
    UHotbarComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static UHotbarComponent* findFor(const APlayerState* player);

    // ---- queries ----
    const TArray<FS_HotbarBinding>& getBindings() const;
    const TArray<FS_InventoryEntry>& getHoldSlots() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Hotbar") FS_HotbarResolve resolveHotbarSlot(int32 slotIndex) const;
    // R01 §5: the full hotbar with the bag, 3 while Bagless.
    UFUNCTION(BlueprintPure, Category = "Awaken|Hotbar") int32 getUsableHotbarSlotCount() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Hotbar") bool isSlotUsable(int32 slotIndex) const;
    // Slot index (1..3) holding this item while Bagless, else INDEX_NONE.
    int32 findHoldSlotIndex(FName itemId, const FGuid& instanceId) const;
    // The instance the hotbar would use next for this item (last used), invalid if none. Display only.
    FGuid getNextForHotbar(FName itemId) const;

    // ---- commands (server only) ----
    FS_InventoryResult bindHotbarSlot(int32 slotIndex, FName itemId);
    FS_InventoryResult selectHotbarSlot(int32 slotIndex);
    // Bagless hold slot: one item, no stacking. Returns the slot index (1..3) or INDEX_NONE when all are full.
    int32 tryAddToHoldSlot(const FS_InventoryEntry& entry);
    FS_InventoryEntry removeFromHoldSlot(int32 slotIndex);
    TArray<FS_InventoryEntry> takeAllHoldSlotItems();
    TArray<FS_HotbarBinding> getBindingsSnapshot() const;
    void restoreBindings(const TArray<FS_HotbarBinding>& newBindings);
    void clearBindings();
    void markInstanceUsed(FName itemId, const FGuid& instanceId);

protected:
    // All three replicate to the owner only (R01 §3.8: hold slots are visible to others only as the item in the hand).
    UPROPERTY(ReplicatedUsing = OnRep_bindings) TArray<FS_HotbarBinding> bindings;
    UPROPERTY(Replicated) TArray<FS_InventoryEntry> holdSlots;      // 3 entries; an empty itemId = free
    UPROPERTY(Replicated) TArray<FS_LastUsedRecord> lastUsed;       // most recent instance per item

    UFUNCTION()
    void OnRep_bindings();

    APlayerState* getOwnerPlayerState() const;
    UInventoryData* getBag() const;
    FGuid pickInstance(FName itemId) const;
    void raiseHotbarChanged(int32 slotIndex) const;

    TMap<FName, TArray<FGuid>> recency;   // server only, most recent first
};
