#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UBFL_Inventory.generated.h"

class UInventoryData;
class UDA_ItemDefinition;
class APlayerState;
class ALooseItemActor;

UCLASS()
class AWAKEN_API UBFL_Inventory : public UBlueprintFunctionLibrary{
    GENERATED_BODY()

public:
    // Query. True on the host, the dedicated server and offline; false on a client.
    static bool isServerWorld(const UObject* worldContext);

    // ---------- Section 4: rules and transfer ----------

    // Query. Empty tag = the move is allowed. Never mutates (R01 §3.4, §3.4a; D9, D10).
    static FGameplayTag checkEntryMove(const UInventoryData* source, const UInventoryData* destination, const FS_InventoryEntry& entry);
    // Query. Drop / throw: Permanent items and non-empty Portable Containers are refused (R01 §3.10, §3.4a).
    static FGameplayTag checkDropOrThrow(const UObject* worldContext, const FS_InventoryEntry& entry);
    // Command, server. Builds a new instance with default state; a Portable Container also gets its nested inventory.
    static FS_InventoryEntry makeFreshInstance(UObject* worldContext, const UDA_ItemDefinition& definition);
    // Command, server. Validates everything, then mutates last (Doc02 §6). Reads AVAILABLE quantity only.
    static FS_InventoryResult transferEntry(UInventoryData* source, UInventoryData* destination, FName itemId, const FGuid& instanceId, int32 quantity);

    // ---------- Section 5: grant, drop, pickup ----------

    static FVector getDropLocation(const APlayerState* player);
    static int64 computeSpoilAt(const UObject* worldContext, const UDA_ItemDefinition& definition);
    // Command, server. Stackable or instanced by definition; creates fresh instances for instanced items.
    static FS_GrantResult grantOrDrop(APlayerState* player, const FS_ItemStack& stack);
    // Command, server. Drops something the player already holds (available quantity only).
    static FS_InventoryResult dropFromInventory(APlayerState* player, FName itemId, const FGuid& instanceId, int32 quantity);
    // Command, server. Loose item -> bag / hands (R01 §3.11). Partial stacks are taken in part.
    static FS_InventoryResult tryPickupLoose(APlayerState* player, ALooseItemActor* loose, int32 quantity);

private:
    static void spawnLoose(UObject* worldContext, const FS_InventoryEntry& entry, const FVector& location);

    // ---------- Section 6 ----------
    // Query. Never empty: Unarmed when nothing is wielded (R01 §3.5, §3.10).
    static FS_HeldItemView getHeldItem(const APlayerState* player);
    // Query. The definition behind getHeldItem; never null while the catalog is loaded.
    static const UDA_ItemDefinition* getHeldDefinition(const APlayerState* player);
    // Query. For R02: "an equipped weapon cannot be staged" (R02 §3.10).
    static bool isInstanceHeld(const APlayerState* player, const FGuid& instanceId);
```

```cpp
#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"

FS_HeldItemView UBFL_Inventory::getHeldItem(const APlayerState* player){
};
