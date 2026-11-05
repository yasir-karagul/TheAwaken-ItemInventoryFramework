#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "ItemInventoryFramework/04Delegates/D_ItemInventoryFramework.h"
#include "UInventoryData.generated.h"

class UDA_ItemDefinition;

// R01 §3.3: an inventory is a data object, not a component, so the bag can move from a player to a bag in the world.
UCLASS(BlueprintType)
class AWAKEN_API UInventoryData : public UObject{
    GENERATED_BODY()

public:
    UInventoryData();

    void initialize(const FGuid& newInventoryId, E_InventoryType newType);

    // ---- UObject ----
    virtual bool IsSupportedForNetworking() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UWorld* GetWorld() const override;

    // ---- identity and rules (Query) ----
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") FGuid getInventoryId() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") E_InventoryType getType() const;
    bool isDeathBag() const;
    int32 getContainerLimit() const;
    const FGameplayTagContainer& getAllowedTags() const;

    // Command, server only. Containers and Portable Containers: set once at creation from data (R01 §3.4).
    void setContainerRules(int32 newLimit, const FGameplayTagContainer& newAllowedTags);
    // Command, server only. A death bag refuses every transfer from other players (R01 §3.4, B02 4.3).
    void setDeathBag(bool newValue);

    // ---- queries ----
    const FS_InventoryEntry* findStackable(FName itemId) const;
    const FS_InventoryEntry* findInstance(const FGuid& instanceId) const;
    const TArray<FS_InventoryEntry>& getEntries() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") TArray<FS_InventoryEntry> getEntriesCopy() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") int32 getTotalQuantity(FName itemId) const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") int32 getReservedQuantity(FName itemId) const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") int32 getAvailableQuantity(FName itemId) const;
    // Instanced: how many copies of this itemId are held / not reserved.
    int32 getInstanceCount(FName itemId) const;
    int32 getAvailableInstanceCount(FName itemId) const;
    bool isEmpty() const;
    int32 getLimitFor(const UDA_ItemDefinition& definition) const;
    // How many of the stack would fit (0 .. stack.quantity). Used by grantOrDrop for partial storing.
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") int32 canFit(const FS_ItemStack& stack) const;

    // ---- commands (server only; each returns an explained result) ----
    FS_InventoryResult addStack(FName itemId, int32 quantity, int64 spoilAtTime = -1);
    FS_InventoryResult removeStack(FName itemId, int32 quantity);
    FS_InventoryResult addInstance(const FS_InventoryEntry& instanceEntry);
    FS_InventoryResult removeInstance(const FGuid& instanceId);
    // Narrow writers so nobody else touches entry state (R01 §3.2). Durability is clamped at 0; unbreakable stays unbreakable.
    FS_InventoryResult setInstanceDurability(const FGuid& instanceId, float newDurability);
    FS_InventoryResult setSpoilAtTime(FName itemId, int64 newSpoilAtTime);
    // Used by the per-viewer mirror (D2). Not a gameplay command.
    FS_InventoryResult replaceEntries(const TArray<FS_InventoryEntry>& newEntries);

    // Called by FS_InventoryEntryArray::PostReplicatedReceive on clients.
    void handleReplicatedChange();

    UPROPERTY(BlueprintAssignable, Category = "Awaken|Inventory") FD_InventoryChanged onInventoryChanged;

    // Replicated payload (public so tests and the save code can read it; mutate only through the commands).
    UPROPERTY(Replicated) FS_InventoryEntryArray entries;

protected:
    bool isServerSide() const;
    void notifyChanged();

    int32 findStackableIndex(FName itemId) const;
    int32 findInstanceIndex(const FGuid& instanceId) const;
    static int64 mergeSpoilTime(int64 existing, int64 incoming);
    static FS_InventoryResult refuse(const FGameplayTag& reason);
    static FS_InventoryResult accept(int32 quantity);

    UPROPERTY(Replicated) FGuid inventoryId;
    UPROPERTY(Replicated) E_InventoryType type = E_InventoryType::Personal;

    // Server-only rules (not replicated; the per-viewer mirror does not need them).
    UPROPERTY() int32 containerLimit = 0;
    UPROPERTY() FGameplayTagContainer allowedTags;
    UPROPERTY() bool deathBag = false;

    // Spoilage (Section 8). Server only. Moves the whole stack to another itemId, ignoring that item's limit.
    FS_SpoilResult convertStack(FName fromItemId, FName toItemId);
```


};
