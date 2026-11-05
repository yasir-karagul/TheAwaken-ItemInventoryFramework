#include "ItemInventoryFramework/UBFL_InventoryConsume.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/PlayerState.h"

namespace{
    FS_InventoryResult refusal(const FGameplayTag& reason){
        FS_InventoryResult result;
        result.failReason = reason;
        return result;
    }
}

FS_SpoilResult UBFL_InventoryConsume::evaluateSpoilage(UInventoryData* inventory, FName itemId){
    FS_SpoilResult none;
    if(inventory == nullptr || !UBFL_Inventory::isServerWorld(inventory)){
        return none;
    }
    const FS_InventoryEntry* entry = inventory->findStackable(itemId);
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(inventory);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
    const UInventoryServices* services = UInventoryServices::get(inventory);
    if(entry == nullptr || definition == nullptr || services == nullptr || !definition->canSpoil || entry->spoilAtTime < 0){
        return none;
    }
    if(services->getGameTimeSeconds() < entry->spoilAtTime){
        return none;   // still fresh
    }
    if(catalog->getDefinition(definition->spoiledItemId) == nullptr){
        INVENTORY_LOG(Error, inventory, "%s spoiled but spoiledItemId %s does not exist", *itemId.ToString(), *definition->spoiledItemId.ToString());
        return none;
    }
    const FS_SpoilResult result = inventory->convertStack(itemId, definition->spoiledItemId);
    INVENTORY_LOG(Log, inventory, "%s spoiled into %s", *itemId.ToString(), *definition->spoiledItemId.ToString());
    return result;
}

TArray<FS_SpoilResult> UBFL_InventoryConsume::evaluateAllSpoilage(UInventoryData* inventory){
    TArray<FS_SpoilResult> results;
    if(inventory == nullptr){
        return results;
    }
    TArray<FName> candidates;
    for(const FS_InventoryEntry& entry : inventory->getEntries()){
        if(!entry.isInstanced() && entry.spoilAtTime >= 0){
            candidates.AddUnique(entry.itemId);
        }
    }
    for(const FName& itemId : candidates){
        const FS_SpoilResult result = evaluateSpoilage(inventory, itemId);
        if(result.converted){
            results.Add(result);
        }
    }
    return results;
}

void UBFL_InventoryConsume::evaluateSpoilageForPlayer(APlayerState* player){
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    if(component == nullptr || !component->hasBag()){
        return;
    }
    for(const FS_SpoilResult& converted : evaluateAllSpoilage(component->getPersonalInventory())){
        component->forgetReservationsForItem(converted.fromItemId);   // R01 §3.2a: the crafting job is refused by R02's normal reason
    }
}

FS_InventoryResult UBFL_InventoryConsume::useConsumable(APlayerState* player, FName itemId, const FGuid& instanceId){
    if(player == nullptr || !UBFL_Inventory::isServerWorld(player)){
        return refusal(T_Inventory_FailReason_NotServer);
    }
    UInventoryServices* services = UInventoryServices::get(player);
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    if(services == nullptr || catalog == nullptr || component == nullptr){
        return refusal(T_Inventory_FailReason_NotFound);
    }

    // 1. State gate (R01 §3.9; Doc B07 3.6: eating and drinking are refused while carrying).
    if(services->isIncapacitated(player)){
        return refusal(T_Interaction_FailReason_NotAllowedInState);
    }
    if(services->isCarryActive(player)){
        return refusal(T_Interaction_FailReason_HandsFull);
    }

    // 2. Spoilage first, so the player eats what is really there (S8.4).
    evaluateSpoilageForPlayer(player);

    const UDA_ItemDefinition* definition = catalog->getDefinition(itemId);
    if(definition == nullptr){
        return refusal(T_Inventory_FailReason_UnknownItem);
    }
    if(!definition->isConsumable()){
        return refusal(T_Inventory_FailReason_NotConsumable);
    }

    // 3. Availability: from the bag, or from a Bagless hold slot (R01 §3.8, Index A110 S2-F33).
    int32 holdSlot = INDEX_NONE;
    UInventoryData* bag = component->getPersonalInventory();
    if(component->hasBag()){
        if(definition->itemKind == E_ItemKind::Instanced){
            const FS_InventoryEntry* instance = bag->findInstance(instanceId);
            if(instance == nullptr){
                return refusal(T_Inventory_FailReason_NotFound);
            }
            if(instance->reservedQuantity > 0){
                return refusal(T_Inventory_FailReason_ReservedQuantity);
            }
        }else{
            const FS_InventoryEntry* stack = bag->findStackable(itemId);
            if(stack == nullptr){
                return refusal(T_Inventory_FailReason_NotFound);
            }
            if(stack->availableQuantity() < 1){
                return refusal(T_Inventory_FailReason_ReservedQuantity);
            }
        }
    }else{
        holdSlot = hotbar != nullptr ? hotbar->findHoldSlotIndex(itemId, instanceId) : INDEX_NONE;
        if(holdSlot == INDEX_NONE){
            return refusal(T_Inventory_FailReason_NotFound);
        }
    }

    // 4. Everything is validated. Work out the effects BEFORE changing state, then remove the unit FIRST (D14).
    const TArray<FS_ConsumeEffect> effects = catalog->getEffectiveConsumeEffects(itemId);
    if(holdSlot != INDEX_NONE){
        hotbar->removeFromHoldSlot(holdSlot);
    }else if(definition->itemKind == E_ItemKind::Instanced){
        bag->removeInstance(instanceId);
    }else{
        bag->removeStack(itemId, 1);
    }

    // 5. Apply the effects.
    const bool illnessActive = services->isMeterActive(T_Survival_Meter_Illness);
    for(const FS_ConsumeEffect& effect : effects){
        switch(effect.kind){
            case E_ConsumeEffectKind::MeterChange:{
                if(services->isMeterActive(effect.tag)){
                    services->modifyMeter(player, effect.tag, effect.amount);
                }
                break;
            }
            case E_ConsumeEffectKind::IllnessChance:{
                // The chance decides only whether the illness source is registered (R01 §3.2a, Index A96 q139).
                if(illnessActive && services->rollChance() < effect.amount){
                    services->registerIllnessSource(player, effect.tag, effect.sourceDurationHours);
                }
                break;
            }
            case E_ConsumeEffectKind::HealthLoss:{
                // Applies every time. Goes through the Doc R03 pipeline with DamageType.Illness (seam).
                if(illnessActive){
                    services->applyIllnessDamage(player, effect.amount);
                }
                break;
            }
        }
    }
    if(definition->healthRestoreAmount > 0.0f){
        services->restoreHealth(player, definition->healthRestoreAmount);   // medicine ignores the regeneration cap (R01 §3.9)
    }
    if(definition->illnessReductionAmount > 0.0f && illnessActive){
        services->modifyMeter(player, T_Survival_Meter_Illness, -definition->illnessReductionAmount);
    }

    FS_InventoryResult result;
    result.quantity = 1;
    return result;
}
