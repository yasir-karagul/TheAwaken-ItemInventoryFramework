#include "ItemInventoryFramework/UBFL_InventoryBag.h"
#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "ItemInventoryFramework/03Interfaces/BPI_BagActorService.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
// S9.10: failure injection for the atomicity test. 0 = off, 1 = fail record/actor creation, 2 = fail gear release.
static TAutoConsoleVariable<int32> cvarFailBagStep(TEXT("Awaken.Inventory.FailBagStep"), 0, TEXT("Force detachBag to fail at step 1 or 2 (tests only)."), ECVF_Cheat);
#endif

namespace{
    bool shouldFailStep(int32 step){
#if !UE_BUILD_SHIPPING
        return cvarFailBagStep.GetValueOnGameThread() == step;
#else
        return false;
#endif
    }

    FS_InventoryResult refusal(const FGameplayTag& reason){
        FS_InventoryResult result;
        result.failReason = reason;
        return result;
    }
}

bool UBFL_InventoryBag::hasBag(const APlayerState* player){
    const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    return component != nullptr && component->hasBag();
}

FS_BagDetachResult UBFL_InventoryBag::detachBag(APlayerState* player, const FVector& deathLocation){
    FS_BagDetachResult result;
    if(player == nullptr || !UBFL_Inventory::isServerWorld(player)){
        result.failReason = T_Inventory_FailReason_NotServer;
        return result;
    }
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    UInventoryServices* services = UInventoryServices::get(player);
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(player);
    UObject* bagService = services != nullptr ? services->getBagService() : nullptr;
    if(component == nullptr || hotbar == nullptr || registry == nullptr || bagService == nullptr){
        result.failReason = T_Inventory_FailReason_NotFound;
        return result;
    }
    if(!component->hasBag()){
        result.failReason = T_Inventory_FailReason_NoBag;   // already Bagless: nothing to detach
        return result;
    }
    UInventoryData* bag = component->getPersonalInventory();
    UHeldItemComponent* held = UHeldItemComponent::findFor(player->GetPawn());

    // Step 0: reservations are released BEFORE the move (Index A103 S4-F28), so the record never holds a reserved quantity.
    component->releaseAllReservations();

    // Step 1: the record and the actor are created first (Doc B02 3.6).
    FGuid bagId;
    if(!shouldFailStep(1)){
        bagId = IBPI_BagActorService::Execute_createBagRecordAndActor(bagService, player, deathLocation, component->getCharacterId(), bag->getInventoryId());
    }
    if(!bagId.IsValid()){
        result.failReason = T_Inventory_FailReason_BagActorFailed;
        return result;   // nothing else moved
    }

    // Step 2: worn armor goes back to the bag record (R05 3.3).
    const FS_GearReleaseResult gear = services->releaseEquippedGear(player);
    if(shouldFailStep(2) || !gear.succeeded){
        IBPI_BagActorService::Execute_deleteBag(bagService, bagId);   // the record/actor created in step 1 are deleted
        result.failReason = T_Inventory_FailReason_GearReleaseFailed;
        return result;
    }

    // Step 3: nothing below can fail.
    FS_BagContents contents;
    contents.personalInventoryId = bag->getInventoryId();
    contents.hotbarSnapshot = hotbar->getBindingsSnapshot();
    contents.gear = gear.gearSlots;
    if(held != nullptr){
        const FS_HeldItemView view = held->getHeldView();
        if(!view.isUnarmed && view.instanceId.IsValid()){
            contents.wieldedInstanceId = view.instanceId;   // listed once: it is already an entry of the bag
        }
    }
    for(const FS_InventoryEntry& gearEntry : gear.gearEntries){
        bag->addInstanceForced(gearEntry);
    }
    UInventoryData* handedOver = component->detachPersonalInventory(registry);
    if(handedOver != nullptr){
        handedOver->setDeathBag(true);
    }
    hotbar->clearBindings();
    if(held != nullptr){
        held->clearToUnarmed();
    }
    IBPI_BagActorService::Execute_finalizeBag(bagService, bagId, contents);

    INVENTORY_LOG(Log, player, "Bag %s detached at death (%d entries, %d gear)", *bagId.ToString(), bag->getEntries().Num(), gear.gearEntries.Num());
    result.bagId = bagId;
    return result;
}

void UBFL_InventoryBag::storeOrDropBesideBag(APlayerState* player, UInventoryData* bag, const FS_InventoryEntry& item, const FVector& location){
    FS_ItemStack single;
    single.itemId = item.itemId;
    single.quantity = 1;
    if(bag != nullptr && bag->canFit(single) > 0){
        const FS_InventoryResult stored = item.isInstanced() ? bag->addInstance(item) : bag->addStack(item.itemId, 1, item.spoilAtTime);
        if(stored.succeeded()){
            return;
        }
    }
    // No room: dropped next to the bag, the player can free a space and take it or leave it (R01 §3.8).
    const UInventoryServices* services = UInventoryServices::get(player);
    if(services != nullptr){
        FS_InventoryEntry looseEntry = item;
        looseEntry.ReplicationID = INDEX_NONE;
        services->spawnLooseItem(player, looseEntry, location + FVector(0.0, 0.0, 50.0));
    }
}

FS_InventoryResult UBFL_InventoryBag::reattachBag(APlayerState* player, const FGuid& bagId){
    if(player == nullptr || !UBFL_Inventory::isServerWorld(player)){
        return refusal(T_Inventory_FailReason_NotServer);
    }
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    UInventoryServices* services = UInventoryServices::get(player);
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(player);
    UObject* bagService = services != nullptr ? services->getBagService() : nullptr;
    if(component == nullptr || hotbar == nullptr || registry == nullptr || bagService == nullptr){
        return refusal(T_Inventory_FailReason_NotFound);
    }

    // Validate everything first.
    if(component->hasBag()){
        return refusal(T_Inventory_FailReason_AlreadyHasBag);
    }
    if(!IBPI_BagActorService::Execute_bagExists(bagService, bagId)){
        return refusal(T_Inventory_FailReason_NotFound);
    }
    if(IBPI_BagActorService::Execute_getBagOwner(bagService, bagId) != component->getCharacterId()){
        return refusal(T_Inventory_FailReason_NotOwner);   // only the owning character can pick the bag up (Doc B02 4.3)
    }
    const FS_BagContents contents = IBPI_BagActorService::Execute_getBagContents(bagService, bagId);
    UInventoryData* bag = registry->findInventory(contents.personalInventoryId);
    if(bag == nullptr){
        return refusal(T_Inventory_FailReason_NotFound);
    }
    const FVector bagLocation = IBPI_BagActorService::Execute_getBagLocation(bagService, bagId);
    UHeldItemComponent* held = UHeldItemComponent::findFor(player->GetPawn());

    // One atomic step (R01 §3.8): (1) restore the inventory, (2) store held items, (3) bindings exactly, (4) re-equip.
    const TArray<FS_InventoryEntry> holdSlotItems = hotbar->takeAllHoldSlotItems();   // hands are emptied first
    if(held != nullptr){
        held->clearToUnarmed();
    }
    bag->setDeathBag(false);
    component->attachPersonalInventory(bag);

    for(const FS_InventoryEntry& item : holdSlotItems){
        storeOrDropBesideBag(player, bag, item, bagLocation);   // never kept in a hold slot; never destroyed
    }
    services->reEquipGear(player, contents.gear);               // R05 re-equips in the slots they occupied
    hotbar->restoreBindings(contents.hotbarSnapshot);           // exactly as at death
    if(held != nullptr && contents.wieldedInstanceId.IsValid()){
        const FS_InventoryEntry* wielded = bag->findInstance(contents.wieldedInstanceId);
        if(wielded != nullptr){
            held->setWielded(wielded->itemId, wielded->instanceId);
        }
    }
    IBPI_BagActorService::Execute_deleteBag(bagService, bagId);

    INVENTORY_LOG(Log, player, "Bag %s recovered", *bagId.ToString());
    FS_InventoryResult result;
    result.quantity = 1;
    return result;
}

int32 UBFL_InventoryBag::discardBaglessItems(APlayerState* player){
    UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    if(player == nullptr || hotbar == nullptr || !UBFL_Inventory::isServerWorld(player)){
        return 0;
    }
    const int32 destroyed = hotbar->takeAllHoldSlotItems().Num();   // destroyed, not dropped (R01 §3.8, Doc B02 10.2)
    UHeldItemComponent* held = UHeldItemComponent::findFor(player->GetPawn());
    if(held != nullptr){
        held->clearToUnarmed();
    }
    return destroyed;
}

FS_InventoryResult UBFL_InventoryBag::grantStartingKit(APlayerState* player){
    if(player == nullptr || !UBFL_Inventory::isServerWorld(player)){
        return refusal(T_Inventory_FailReason_NotServer);
    }
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    if(component == nullptr || catalog == nullptr || !component->hasBag()){
        return refusal(T_Inventory_FailReason_NoBag);
    }
    if(component->isStartingKitGranted()){
        return refusal(T_Inventory_FailReason_AlreadyHasBag);   // never twice (Index A93 UD21)
    }
    UInventoryData* bag = component->getPersonalInventory();
    int32 granted = 0;
    for(const FName& kitItemId : UItemInventorySettings::get()->startingKitItemIds){
        const UDA_ItemDefinition* definition = catalog->getDefinition(kitItemId);
        if(definition == nullptr){
            continue;
        }
        if(definition->itemKind == E_ItemKind::Instanced){
            if(bag->addInstance(UBFL_Inventory::makeFreshInstance(player, *definition)).succeeded()){
                granted++;
            }
        }else if(bag->addStack(kitItemId, 1).succeeded()){
            granted++;
        }
    }
    component->markStartingKitGranted();
    FS_InventoryResult result;
    result.quantity = granted;
    return result;
}

FS_InventoryResult UBFL_InventoryBag::startNewCharacter(APlayerState* player, const FGuid& newCharacterId){
    if(player == nullptr || !UBFL_Inventory::isServerWorld(player)){
        return refusal(T_Inventory_FailReason_NotServer);
    }
    UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    if(component == nullptr || hotbar == nullptr){
        return refusal(T_Inventory_FailReason_NotFound);
    }
    // The old character's bag is lost for good: nobody else can pick it up, the new character does not own it (R01 §3.8).
    if(component->getCharacterId().IsValid()){
        removeBagsOwnedBy(player, component->getCharacterId());
    }
    hotbar->takeAllHoldSlotItems();
    hotbar->clearBindings();
    UHeldItemComponent* held = UHeldItemComponent::findFor(player->GetPawn());
    if(held != nullptr){
        held->clearToUnarmed();
    }
    component->resetForNewCharacter(newCharacterId);
    return grantStartingKit(player);
}

void UBFL_InventoryBag::removeBagsOwnedBy(const UObject* worldContext, const FGuid& characterId){
    const UInventoryServices* services = UInventoryServices::get(worldContext);
    UObject* bagService = services != nullptr ? services->getBagService() : nullptr;
    if(bagService != nullptr){
        IBPI_BagActorService::Execute_removeBagsOwnedBy(bagService, characterId);
    }
}
