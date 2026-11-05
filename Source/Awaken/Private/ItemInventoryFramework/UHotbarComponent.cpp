#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UHotbarComponent::UHotbarComponent(){
    SetIsReplicatedByDefault(true);
}

void UHotbarComponent::BeginPlay(){
    Super::BeginPlay();
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        bindings.SetNum(ItemInventoryConstants::hotbarSlotCount);
        for(int32 index = 0; index < bindings.Num(); index++){
            bindings[index].slotIndex = index;
        }
        holdSlots.SetNum(ItemInventoryConstants::baglessHoldSlotCount);
    }
}

void UHotbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UHotbarComponent, bindings, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UHotbarComponent, holdSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UHotbarComponent, lastUsed, COND_OwnerOnly);
}

UHotbarComponent* UHotbarComponent::findFor(const APlayerState* player){
    return player != nullptr ? player->FindComponentByClass<UHotbarComponent>() : nullptr;
}

APlayerState* UHotbarComponent::getOwnerPlayerState() const{
    return Cast<APlayerState>(GetOwner());
}

UInventoryData* UHotbarComponent::getBag() const{
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(getOwnerPlayerState());
    return inventory != nullptr ? inventory->getPersonalInventory() : nullptr;
}

const TArray<FS_HotbarBinding>& UHotbarComponent::getBindings() const{
    return bindings;
}

const TArray<FS_InventoryEntry>& UHotbarComponent::getHoldSlots() const{
    return holdSlots;
}

int32 UHotbarComponent::getUsableHotbarSlotCount() const{
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(getOwnerPlayerState());
    const bool bagless = inventory == nullptr || !inventory->hasBag();
    return bagless ? ItemInventoryConstants::baglessHoldSlotCount : ItemInventoryConstants::hotbarSlotCount;
}

bool UHotbarComponent::isSlotUsable(int32 slotIndex) const{
    if(slotIndex < 0 || slotIndex >= ItemInventoryConstants::hotbarSlotCount){
        return false;
    }
    if(getUsableHotbarSlotCount() == ItemInventoryConstants::hotbarSlotCount){
        return true;
    }
    return slotIndex >= 1 && slotIndex <= ItemInventoryConstants::baglessHoldSlotCount;   // D12
}

int32 UHotbarComponent::findHoldSlotIndex(FName itemId, const FGuid& instanceId) const{
    for(int32 index = 0; index < holdSlots.Num(); index++){
        const FS_InventoryEntry& entry = holdSlots[index];
        if(entry.itemId == itemId && (!instanceId.IsValid() || entry.instanceId == instanceId)){
            return index + 1;
        }
    }
    return INDEX_NONE;
}

FGuid UHotbarComponent::pickInstance(FName itemId) const{
    const UInventoryData* bag = getBag();
    if(bag == nullptr){
        return FGuid();
    }
    // Preferred order: server recency list; a client only knows the single last-used record.
    TArray<FGuid> order;
    const TArray<FGuid>* known = recency.Find(itemId);
    if(known != nullptr){
        order = *known;
    }else{
        for(const FS_LastUsedRecord& record : lastUsed){
            if(record.itemId == itemId){
                order.Add(record.instanceId);
            }
        }
    }
    for(const FGuid& candidate : order){
        const FS_InventoryEntry* entry = bag->findInstance(candidate);
        if(entry != nullptr && entry->reservedQuantity == 0){
            return candidate;   // the next most recently used takes over when one breaks (R01 §3.5)
        }
    }
    for(const FS_InventoryEntry& entry : bag->getEntries()){
        if(entry.isInstanced() && entry.itemId == itemId && entry.reservedQuantity == 0){
            return entry.instanceId;
        }
    }
    return FGuid();
}

FGuid UHotbarComponent::getNextForHotbar(FName itemId) const{
    return pickInstance(itemId);
}

FS_HotbarResolve UHotbarComponent::resolveHotbarSlot(int32 slotIndex) const{
    FS_HotbarResolve resolved;
    if(!isSlotUsable(slotIndex)){
        return resolved;
    }
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(getOwnerPlayerState());
    const bool bagless = inventory == nullptr || !inventory->hasBag();

    if(bagless){
        const int32 holdIndex = slotIndex - 1;
        if(holdSlots.IsValidIndex(holdIndex) && !holdSlots[holdIndex].itemId.IsNone()){
            resolved.valid = true;
            resolved.itemId = holdSlots[holdIndex].itemId;
            resolved.instanceId = holdSlots[holdIndex].instanceId;
            resolved.availableQuantity = 1;
        }
        return resolved;
    }

    if(!bindings.IsValidIndex(slotIndex) || bindings[slotIndex].boundItemId.IsNone()){
        return resolved;
    }
    const FName itemId = bindings[slotIndex].boundItemId;
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
    const UInventoryData* bag = inventory->getPersonalInventory();
    if(definition == nullptr || bag == nullptr){
        return resolved;
    }
    if(definition->itemKind == E_ItemKind::Instanced){
        const FGuid instanceId = pickInstance(itemId);
        if(instanceId.IsValid()){
            resolved.valid = true;
            resolved.itemId = itemId;
            resolved.instanceId = instanceId;
            resolved.availableQuantity = bag->getAvailableInstanceCount(itemId);
        }
    }else{
        const int32 available = bag->getAvailableQuantity(itemId);   // shared pool: two slots, one stack
        if(available > 0){
            resolved.valid = true;
            resolved.itemId = itemId;
            resolved.availableQuantity = available;
        }
    }
    return resolved;
}

FS_InventoryResult UHotbarComponent::bindHotbarSlot(int32 slotIndex, FName itemId){
    FS_InventoryResult result;
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        result.failReason = T_Inventory_FailReason_NotServer;
        return result;
    }
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(getOwnerPlayerState());
    if(inventory == nullptr || !inventory->hasBag()){
        result.failReason = T_Inventory_FailReason_NoBag;   // a Bagless player has no bindings (R01 §3.8)
        return result;
    }
    if(!isSlotUsable(slotIndex) || !bindings.IsValidIndex(slotIndex)){
        result.failReason = T_Inventory_FailReason_InvalidSlot;
        return result;
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    if(catalog == nullptr || catalog->getDefinition(itemId) == nullptr){
        result.failReason = T_Inventory_FailReason_UnknownItem;
        return result;
    }
    bindings[slotIndex].boundItemId = itemId;   // replaces any earlier binding on this slot
    raiseHotbarChanged(slotIndex);
    result.quantity = 1;
    return result;
}

FS_InventoryResult UHotbarComponent::selectHotbarSlot(int32 slotIndex){
    FS_InventoryResult result;
    APlayerState* player = getOwnerPlayerState();
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority() || player == nullptr){
        result.failReason = T_Inventory_FailReason_NotServer;
        return result;
    }
    if(!isSlotUsable(slotIndex)){
        result.failReason = T_Inventory_FailReason_InvalidSlot;
        return result;
    }
    const UInventoryServices* services = UInventoryServices::get(this);
    if(services != nullptr && services->isIncapacitated(player)){
        result.failReason = T_Interaction_FailReason_NotAllowedInState;
        return result;
    }
    if(services != nullptr && services->isCarryActive(player)){
        result.failReason = T_Interaction_FailReason_HandsFull;   // equipping is refused while carrying (Doc B07 3.6)
        return result;
    }
    APawn* pawn = player->GetPawn();
    UHeldItemComponent* held = UHeldItemComponent::findFor(pawn);
    if(held == nullptr){
        result.failReason = T_Inventory_FailReason_NotFound;
        return result;
    }

    const FS_HotbarResolve resolved = resolveHotbarSlot(slotIndex);
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(player);
    const bool bagless = inventory == nullptr || !inventory->hasBag();
    if(!resolved.valid){
        held->clearToUnarmed();   // selecting an empty slot leaves the hands empty: Unarmed
    }else if(bagless){
        held->clearToUnarmed();
        held->setDisplayOnly(resolved.itemId);   // shown in the hand, not wielded (Index A110 Q-S2-10)
    }else{
        if(resolved.instanceId.IsValid()){
            markInstanceUsed(resolved.itemId, resolved.instanceId);
        }
        held->setWielded(resolved.itemId, resolved.instanceId);
    }
    raiseHotbarChanged(slotIndex);
    result.quantity = 1;
    return result;
}

int32 UHotbarComponent::tryAddToHoldSlot(const FS_InventoryEntry& entry){
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return INDEX_NONE;
    }
    for(int32 index = 0; index < holdSlots.Num(); index++){
        if(holdSlots[index].itemId.IsNone()){
            holdSlots[index] = entry;
            holdSlots[index].totalQuantity = 1;   // exactly one item per slot, no stacking
            holdSlots[index].reservedQuantity = 0;
            raiseHotbarChanged(index + 1);
            return index + 1;
        }
    }
    return INDEX_NONE;
}

FS_InventoryEntry UHotbarComponent::removeFromHoldSlot(int32 slotIndex){
    FS_InventoryEntry removed;
    const int32 holdIndex = slotIndex - 1;
    if(GetOwner() != nullptr && GetOwner()->HasAuthority() && holdSlots.IsValidIndex(holdIndex)){
        removed = holdSlots[holdIndex];
        holdSlots[holdIndex] = FS_InventoryEntry();
        raiseHotbarChanged(slotIndex);
    }
    return removed;
}

TArray<FS_InventoryEntry> UHotbarComponent::takeAllHoldSlotItems(){
    TArray<FS_InventoryEntry> items;
    for(int32 index = 0; index < holdSlots.Num(); index++){
        if(!holdSlots[index].itemId.IsNone()){
            items.Add(holdSlots[index]);
            holdSlots[index] = FS_InventoryEntry();
        }
    }
    return items;
}

TArray<FS_HotbarBinding> UHotbarComponent::getBindingsSnapshot() const{
    return bindings;
}

void UHotbarComponent::restoreBindings(const TArray<FS_HotbarBinding>& newBindings){
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    clearBindings();
    for(const FS_HotbarBinding& binding : newBindings){
        if(bindings.IsValidIndex(binding.slotIndex)){
            bindings[binding.slotIndex].boundItemId = binding.boundItemId;
        }
    }
    raiseHotbarChanged(INDEX_NONE);
}

void UHotbarComponent::clearBindings(){
    for(FS_HotbarBinding& binding : bindings){
        binding.boundItemId = NAME_None;
    }
}

void UHotbarComponent::markInstanceUsed(FName itemId, const FGuid& instanceId){
    TArray<FGuid>& order = recency.FindOrAdd(itemId);
    order.Remove(instanceId);
    order.Insert(instanceId, 0);
    bool updated = false;
    for(FS_LastUsedRecord& record : lastUsed){
        if(record.itemId == itemId){
            record.instanceId = instanceId;
            updated = true;
        }
    }
    if(!updated){
        FS_LastUsedRecord record;
        record.itemId = itemId;
        record.instanceId = instanceId;
        lastUsed.Add(record);
    }
}

void UHotbarComponent::OnRep_bindings(){
    raiseHotbarChanged(INDEX_NONE);
}

void UHotbarComponent::raiseHotbarChanged(int32 slotIndex) const{
    FS_HotbarChangedMessage message;
    message.player = getOwnerPlayerState();
    message.slotIndex = slotIndex;
    UGameplayMessageSubsystem::Get(this).BroadcastMessage(T_Hotbar_Changed, message);
}
