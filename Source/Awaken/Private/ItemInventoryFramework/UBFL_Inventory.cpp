#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/ALooseItemActor.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace{
    FS_InventoryResult makeRefusal(const FGameplayTag& reason){
        FS_InventoryResult result;
        result.failReason = reason;
        return result;
    }
}

bool UBFL_Inventory::isServerWorld(const UObject* worldContext){
    if(GEngine == nullptr || worldContext == nullptr){
        return false;
    }
    const UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
    return world != nullptr && world->GetNetMode() != NM_Client;
}

FGameplayTag UBFL_Inventory::checkEntryMove(const UInventoryData* source, const UInventoryData* destination, const FS_InventoryEntry& entry){
    if(source == nullptr || destination == nullptr){
        return T_Inventory_FailReason_NotFound;
    }
    if(source->isDeathBag() || destination->isDeathBag()){
        return T_Inventory_FailReason_DeathBagLocked;   // R01 §3.4, Doc B02 4.3
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(source);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(entry.itemId) : nullptr;
    if(definition == nullptr){
        return T_Inventory_FailReason_UnknownItem;
    }
    if(definition->isHeavy()){
        return T_Inventory_FailReason_HeavyItem;
    }
    if(destination->getType() == E_InventoryType::Personal){
        return FGameplayTag();   // coming back to the bag is always fine
    }

    // Destination is a Container or a Portable Container.
    if(definition->isPermanent()){
        return T_Inventory_FailReason_Permanent;   // Axe and map never leave the bag (R01 §3.10)
    }
    const FGameplayTagContainer& allowed = destination->getAllowedTags();
    if(!allowed.IsEmpty() && !definition->itemTags.HasAny(allowed) && !allowed.HasTag(definition->categoryTag)){
        return T_Inventory_FailReason_ItemNotAllowed;
    }
    // One water kind per container (R01 §3.4).
    const bool isClean = definition->itemTags.HasTag(T_Item_Water_Clean);
    const bool isUnclean = definition->itemTags.HasTag(T_Item_Water_Unclean);
    if(isClean || isUnclean){
        for(const FS_InventoryEntry& held : destination->getEntries()){
            const UDA_ItemDefinition* heldDefinition = catalog->getDefinition(held.itemId);
            if(heldDefinition == nullptr){
                continue;
            }
            if((isClean && heldDefinition->itemTags.HasTag(T_Item_Water_Unclean)) || (isUnclean && heldDefinition->itemTags.HasTag(T_Item_Water_Clean))){
                return T_Inventory_FailReason_MixedWater;
            }
        }
    }
    // Portable Containers (D9).
    if(definition->isPortableContainer()){
        if(destination->getType() == E_InventoryType::PortableContainer){
            return T_Inventory_FailReason_NestedContainer;
        }
        const UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(source);
        const UInventoryData* contents = registry != nullptr ? registry->findInventory(entry.state.containedInventoryId) : nullptr;
        if(contents != nullptr && !contents->isEmpty()){
            return T_Inventory_FailReason_ContainerNotEmpty;
        }
    }
    return FGameplayTag();
}

FGameplayTag UBFL_Inventory::checkDropOrThrow(const UObject* worldContext, const FS_InventoryEntry& entry){
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(worldContext);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(entry.itemId) : nullptr;
    if(definition == nullptr){
        return T_Inventory_FailReason_UnknownItem;
    }
    if(definition->isPermanent()){
        return T_Inventory_FailReason_Permanent;
    }
    if(definition->isPortableContainer()){
        const UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(worldContext);
        const UInventoryData* contents = registry != nullptr ? registry->findInventory(entry.state.containedInventoryId) : nullptr;
        if(contents != nullptr && !contents->isEmpty()){
            return T_Inventory_FailReason_ContainerNotEmpty;
        }
    }
    return FGameplayTag();
}

FS_InventoryEntry UBFL_Inventory::makeFreshInstance(UObject* worldContext, const UDA_ItemDefinition& definition){
    FS_InventoryEntry entry;
    entry.itemId = definition.itemId;
    entry.instanceId = FGuid::NewGuid();
    entry.totalQuantity = 1;
    entry.state.durability = definition.durabilityMax;   // unbreakable keeps the sentinel
    if(definition.isPortableContainer()){
        UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(worldContext);
        UInventoryData* nested = registry != nullptr ? registry->createInventory(E_InventoryType::PortableContainer) : nullptr;
        if(nested != nullptr){
            nested->setContainerRules(definition.containerPerTypeLimit, definition.allowedTags);
            entry.state.containedInventoryId = nested->getInventoryId();
        }
    }
    return entry;
}

FS_InventoryResult UBFL_Inventory::transferEntry(UInventoryData* source, UInventoryData* destination, FName itemId, const FGuid& instanceId, int32 quantity){
    if(source == nullptr || destination == nullptr){
        return makeRefusal(T_Inventory_FailReason_NotFound);
    }
    if(!isServerWorld(source)){
        return makeRefusal(T_Inventory_FailReason_NotServer);
    }
    if(source == destination || quantity <= 0){
        return makeRefusal(T_Inventory_FailReason_InvalidQuantity);
    }
    // S8.4 adds here: evaluate spoilage on source and destination before reading them.

    const FS_InventoryEntry* found = instanceId.IsValid() ? source->findInstance(instanceId) : source->findStackable(itemId);
    if(found == nullptr){
        return makeRefusal(T_Inventory_FailReason_NotFound);
    }
    if(found->isInstanced()){
        if(quantity != 1){
            return makeRefusal(T_Inventory_FailReason_InvalidQuantity);
        }
        if(found->reservedQuantity > 0){
            return makeRefusal(T_Inventory_FailReason_ReservedQuantity);
        }
    }else if(quantity > found->availableQuantity()){
        return makeRefusal(quantity <= found->totalQuantity ? T_Inventory_FailReason_ReservedQuantity : T_Inventory_FailReason_InsufficientQuantity);
    }

    const FS_InventoryEntry snapshot = *found;   // copy: the source entry is about to change
    const FGameplayTag ruleFailure = checkEntryMove(source, destination, snapshot);
    if(ruleFailure.IsValid()){
        return makeRefusal(ruleFailure);
    }
    FS_ItemStack stack;
    stack.itemId = snapshot.itemId;
    stack.quantity = quantity;
    if(destination->canFit(stack) < quantity){
        return makeRefusal(T_Inventory_FailReason_LimitReached);
    }

    // All checks passed: mutate last. A merge keeps the earliest spoil time; a split copies it (R01 §3.2a).
    if(snapshot.isInstanced()){
        source->removeInstance(snapshot.instanceId);
        const FS_InventoryResult added = destination->addInstance(snapshot);
        if(!added.succeeded()){
            source->addInstance(snapshot);   // roll back; cannot normally happen after the checks above
            return added;
        }
    }else{
        source->removeStack(snapshot.itemId, quantity);
        const FS_InventoryResult added = destination->addStack(snapshot.itemId, quantity, snapshot.spoilAtTime);
        if(!added.succeeded()){
            source->addStack(snapshot.itemId, quantity, snapshot.spoilAtTime);
            return added;
        }
    }
    FS_InventoryResult result;
    result.quantity = quantity;
    return result;
}
