#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UBFL_InventoryConsume.h"
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
    UBFL_InventoryConsume::evaluateAllSpoilage(source);
    UBFL_InventoryConsume::evaluateAllSpoilage(destination);

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


// ---------- Section 5 ----------

FVector UBFL_Inventory::getDropLocation(const APlayerState* player){
    const float forwardOffset = UItemInventorySettings::get()->dropForwardOffset;
    const APawn* pawn = player != nullptr ? player->GetPawn() : nullptr;
    if(pawn == nullptr){
        return player != nullptr ? player->GetActorLocation() : FVector::ZeroVector;
    }
    return pawn->GetActorLocation() + pawn->GetActorForwardVector() * forwardOffset + FVector(0.0, 0.0, 50.0);
}

int64 UBFL_Inventory::computeSpoilAt(const UObject* worldContext, const UDA_ItemDefinition& definition){
    if(!definition.canSpoil){
        return -1;
    }
    const UInventoryServices* services = UInventoryServices::get(worldContext);
    const int64 now = services != nullptr ? services->getGameTimeSeconds() : 0;
    return now + static_cast<int64>(definition.spoilTime * static_cast<float>(ItemInventoryConstants::gameSecondsPerGameHour));
}

void UBFL_Inventory::spawnLoose(UObject* worldContext, const FS_InventoryEntry& entry, const FVector& location){
    const UInventoryServices* services = UInventoryServices::get(worldContext);
    if(services != nullptr){
        services->spawnLooseItem(worldContext, entry, location);
    }
}

FS_GrantResult UBFL_Inventory::grantOrDrop(APlayerState* player, const FS_ItemStack& stack){
    FS_GrantResult result;
    if(player == nullptr || !isServerWorld(player)){
        result.failReason = T_Inventory_FailReason_NotServer;
        return result;
    }
    if(stack.quantity <= 0){
        result.failReason = T_Inventory_FailReason_InvalidQuantity;
        return result;
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(stack.itemId) : nullptr;
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UInventoryServices* services = UInventoryServices::get(player);
    if(definition == nullptr || component == nullptr || services == nullptr){
        result.failReason = definition == nullptr ? T_Inventory_FailReason_UnknownItem : T_Inventory_FailReason_NotFound;
        return result;
    }

    const FVector dropLocation = getDropLocation(player);
    int32 remaining = stack.quantity;

    if(definition->isHeavy()){
        // Step 0: a carry item goes to a free carry slot, else lands loose beside the player. Never in the bag.
        while(remaining > 0 && services->hasFreeCarrySlot(player, definition->carryCapacityCost)){
            if(!services->beginCarry(player, stack.itemId)){
                break;
            }
            remaining--;
            result.quantityCarried++;
        }
    }else if(component->hasBag()){
        // Step 1: store up to the per-type limit.
        UInventoryData* bag = component->getPersonalInventory();
        FS_ItemStack wanted;
        wanted.itemId = stack.itemId;
        wanted.quantity = remaining;
        const int32 fit = bag->canFit(wanted);
        if(fit > 0){
            if(definition->itemKind == E_ItemKind::Stackable){
                if(bag->addStack(stack.itemId, fit).succeeded()){
                    result.quantityStored = fit;
                    remaining -= fit;
                }
            }else{
                for(int32 count = 0; count < fit; count++){
                    if(bag->addInstance(makeFreshInstance(player, *definition)).succeeded()){
                        result.quantityStored++;
                        remaining--;
                    }
                }
            }
        }
    }
    else{
        // Bagless: one unit into a free hold slot first (R01 §3.7, §3.8), the rest drops below.
        UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
        if(hotbar != nullptr && remaining > 0){
            FS_InventoryEntry single;
            if(definition->itemKind == E_ItemKind::Instanced){
                single = makeFreshInstance(player, *definition);
            }else{
                single.itemId = stack.itemId;
                single.totalQuantity = 1;
                single.spoilAtTime = computeSpoilAt(player, *definition);
            }
            if(hotbar->tryAddToHoldSlot(single) != INDEX_NONE){
                remaining--;
                result.quantityStored++;
            }
        }
    }

    // Step 2: drop the rest as loose items. An item is never destroyed (Doc02 §19.5).
    if(remaining > 0){
        if(definition->itemKind == E_ItemKind::Stackable && !definition->isHeavy()){
            FS_InventoryEntry looseEntry;
            looseEntry.itemId = stack.itemId;
            looseEntry.totalQuantity = remaining;
            looseEntry.spoilAtTime = computeSpoilAt(player, *definition);
            spawnLoose(player, looseEntry, dropLocation);
        }else{
            // Heavy and instanced items: one loose body per unit (one heavy item per carry slot, R01 §3.7).
            for(int32 count = 0; count < remaining; count++){
                FS_InventoryEntry looseEntry;
                if(definition->itemKind == E_ItemKind::Instanced){
                    looseEntry = makeFreshInstance(player, *definition);
                }else{
                    looseEntry.itemId = stack.itemId;
                    looseEntry.totalQuantity = 1;
                }
                spawnLoose(player, looseEntry, dropLocation);
            }
        }
        result.quantityDropped = remaining;
    }

    // Step 3: tell the screen. Delivery to the owning client is wired in Section 12 (S12.5).
    if(result.quantityDropped > 0){
        FS_GrantDroppedMessage message;
        message.player = player;
        message.itemId = stack.itemId;
        message.quantity = result.quantityDropped;
        UGameplayMessageSubsystem::Get(player).BroadcastMessage(T_Inventory_GrantDropped, message);
    // inside "if(result.quantityDropped > 0){" after the BroadcastMessage call:
    UInventoryRequestComponent* requests = UInventoryRequestComponent::findFor(player);
    const APlayerController* controller = Cast<APlayerController>(player->GetOwner());
    if(requests != nullptr && controller != nullptr && !controller->IsLocalController()){
        requests->clientGrantDropped(stack.itemId, result.quantityDropped);
    }
        INVENTORY_LOG(Log, player, "grantOrDrop %s x%d: stored %d, carried %d, dropped %d", *stack.itemId.ToString(), stack.quantity, result.quantityStored, result.quantityCarried, result.quantityDropped);
        // B21 hook: raise Cosmetic.Event.Interaction.ItemDropped here when the cosmetic channel exists.
    }
    return result;
}

FS_InventoryResult UBFL_Inventory::dropFromInventory(APlayerState* player, FName itemId, const FGuid& instanceId, int32 quantity){
    if(player == nullptr || !isServerWorld(player)){
        return makeRefusal(T_Inventory_FailReason_NotServer);
    }
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    if(component == nullptr || !component->hasBag()){
        return makeRefusal(T_Inventory_FailReason_NoBag);
    }
    UInventoryData* bag = component->getPersonalInventory();
    const FS_InventoryEntry* found = instanceId.IsValid() ? bag->findInstance(instanceId) : bag->findStackable(itemId);
    if(found == nullptr){
        return makeRefusal(T_Inventory_FailReason_NotFound);
    }
    if(quantity <= 0 || (found->isInstanced() && quantity != 1)){
        return makeRefusal(T_Inventory_FailReason_InvalidQuantity);
    }
    if(!found->isInstanced() && quantity > found->availableQuantity()){
        return makeRefusal(quantity <= found->totalQuantity ? T_Inventory_FailReason_ReservedQuantity : T_Inventory_FailReason_InsufficientQuantity);
    }
    if(found->isInstanced() && found->reservedQuantity > 0){
        return makeRefusal(T_Inventory_FailReason_ReservedQuantity);
    }
    const FS_InventoryEntry snapshot = *found;
    const FGameplayTag ruleFailure = checkDropOrThrow(player, snapshot);
    if(ruleFailure.IsValid()){
        return makeRefusal(ruleFailure);
    }

    // Validated: change state last.
    FS_InventoryEntry looseEntry = snapshot;
    looseEntry.ReplicationID = INDEX_NONE;
    looseEntry.reservedQuantity = 0;
    looseEntry.totalQuantity = snapshot.isInstanced() ? 1 : quantity;
    if(snapshot.isInstanced()){
        bag->removeInstance(snapshot.instanceId);
    }else{
        bag->removeStack(snapshot.itemId, quantity);
    }
    spawnLoose(player, looseEntry, getDropLocation(player));
    FS_InventoryResult result;
    result.quantity = quantity;
    return result;
}

FS_InventoryResult UBFL_Inventory::tryPickupLoose(APlayerState* player, ALooseItemActor* loose, int32 quantity){
    if(player == nullptr || loose == nullptr || !isServerWorld(player)){
        return makeRefusal(T_Inventory_FailReason_NotServer);
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    const FS_InventoryEntry entry = loose->getEntry();
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(entry.itemId) : nullptr;
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UInventoryServices* services = UInventoryServices::get(player);
    if(definition == nullptr || component == nullptr || services == nullptr || entry.totalQuantity <= 0){
        return makeRefusal(T_Inventory_FailReason_NotFound);
    }
    const int32 wanted = quantity <= 0 ? entry.totalQuantity : FMath::Min(quantity, entry.totalQuantity);

    // Heavy item: hands only (R01 §3.11).
    if(definition->isHeavy()){
        if(!services->hasFreeCarrySlot(player, definition->carryCapacityCost)){
            return makeRefusal(T_Interaction_FailReason_HandsFull);
        }
        if(!services->beginCarry(player, entry.itemId)){
            return makeRefusal(T_Interaction_FailReason_HandsFull);
        }
        loose->consumeUnits(1);
        FS_InventoryResult carried;
        carried.quantity = 1;
        return carried;
    }

    if(!component->hasBag()){
        UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
        FS_InventoryEntry single = entry;
        single.ReplicationID = INDEX_NONE;
        single.totalQuantity = 1;
        if(hotbar == nullptr || hotbar->tryAddToHoldSlot(single) == INDEX_NONE){
            // All three hold slots full: a short reason from the existing refusal-line pattern (Index A110 S2-F14).
            return makeRefusal(T_Interaction_FailReason_BagFull);
        }
        loose->consumeUnits(1);
        FS_InventoryResult taken;
        taken.quantity = 1;
        return taken;
    }

    UInventoryData* bag = component->getPersonalInventory();
    FS_ItemStack stack;
    stack.itemId = entry.itemId;
    stack.quantity = wanted;
    const int32 fit = bag->canFit(stack);
    if(fit <= 0){
        // At the type limit: refuse and drop nothing, so there is no pick-up/drop loop (R01 §3.11).
        return makeRefusal(T_Interaction_FailReason_BagFull);
    }

    // Take the part that fits; the rest stays where it lies.
    FS_InventoryResult added;
    if(entry.isInstanced()){
        added = bag->addInstance(entry);
        if(added.succeeded()){
            loose->consumeUnits(1);
        }
    }else{
        added = bag->addStack(entry.itemId, fit, entry.spoilAtTime);
        if(added.succeeded()){
            loose->consumeUnits(fit);
        }
    }
    return added;
}


#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"

FS_HeldItemView UBFL_Inventory::getHeldItem(const APlayerState* player){
    const UHeldItemComponent* held = UHeldItemComponent::findFor(player != nullptr ? player->GetPawn() : nullptr);
    if(held == nullptr){
        FS_HeldItemView unarmed;
        unarmed.itemId = UItemInventorySettings::get()->unarmedItemId;
        return unarmed;
    }
    return held->getHeldView();
}

const UDA_ItemDefinition* UBFL_Inventory::getHeldDefinition(const APlayerState* player){
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    if(catalog == nullptr){
        return nullptr;
    }
    const UDA_ItemDefinition* definition = catalog->getDefinition(getHeldItem(player).itemId);
    return definition != nullptr ? definition : catalog->getDefinition(UItemInventorySettings::get()->unarmedItemId);
}

bool UBFL_Inventory::isInstanceHeld(const APlayerState* player, const FGuid& instanceId){
    const UHeldItemComponent* held = UHeldItemComponent::findFor(player != nullptr ? player->GetPawn() : nullptr);
    return held != nullptr && held->isInstanceHeld(instanceId);
}
