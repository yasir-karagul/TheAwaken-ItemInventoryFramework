#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

// ---------- FS_InventoryEntryArray hook (declared in S_ItemInventoryFramework.h) ----------

void FS_InventoryEntryArray::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& parameters){
    if(owner != nullptr){
        owner->handleReplicatedChange();
    }
}

// ---------- construction and identity ----------

UInventoryData::UInventoryData(){
    entries.owner = this;   // set here so a client-side copy created by replication also has it
}

void UInventoryData::initialize(const FGuid& newInventoryId, E_InventoryType newType){
    inventoryId = newInventoryId;
    type = newType;
}

bool UInventoryData::IsSupportedForNetworking() const{
    return true;
}

void UInventoryData::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryData, entries);
    DOREPLIFETIME_CONDITION(UInventoryData, inventoryId, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(UInventoryData, type, COND_InitialOnly);
}

UWorld* UInventoryData::GetWorld() const{
    const UObject* outer = GetOuter();
    return outer != nullptr ? outer->GetWorld() : nullptr;
}

FGuid UInventoryData::getInventoryId() const{
    return inventoryId;
}

E_InventoryType UInventoryData::getType() const{
    return type;
}

bool UInventoryData::isDeathBag() const{
    return deathBag;
}

int32 UInventoryData::getContainerLimit() const{
    return containerLimit;
}

const FGameplayTagContainer& UInventoryData::getAllowedTags() const{
    return allowedTags;
}

void UInventoryData::setContainerRules(int32 newLimit, const FGameplayTagContainer& newAllowedTags){
    if(!isServerSide()){
        return;
    }
    containerLimit = newLimit;
    allowedTags = newAllowedTags;
}

void UInventoryData::setDeathBag(bool newValue){
    if(isServerSide()){
        deathBag = newValue;
    }
}

bool UInventoryData::isServerSide() const{
    const UWorld* world = GetWorld();
    return world != nullptr && world->GetNetMode() != NM_Client;
}

// ---------- helpers ----------

FS_InventoryResult UInventoryData::refuse(const FGameplayTag& reason){
    FS_InventoryResult result;
    result.failReason = reason;
    return result;
}

FS_InventoryResult UInventoryData::accept(int32 quantity){
    FS_InventoryResult result;
    result.quantity = quantity;
    return result;
}

int64 UInventoryData::mergeSpoilTime(int64 existing, int64 incoming){
    // R01 §3.2a: merging keeps the earliest (worst) time. -1 means "does not spoil".
    if(existing < 0){
        return incoming;
    }
    if(incoming < 0){
        return existing;
    }
    return FMath::Min(existing, incoming);
}

int32 UInventoryData::findStackableIndex(FName itemId) const{
    for(int32 index = 0; index < entries.items.Num(); index++){
        const FS_InventoryEntry& entry = entries.items[index];
        if(!entry.isInstanced() && entry.itemId == itemId){
            return index;
        }
    }
    return INDEX_NONE;
}

int32 UInventoryData::findInstanceIndex(const FGuid& instanceId) const{
    for(int32 index = 0; index < entries.items.Num(); index++){
        if(entries.items[index].instanceId == instanceId){
            return index;
        }
    }
    return INDEX_NONE;
}

void UInventoryData::notifyChanged(){
    onInventoryChanged.Broadcast(inventoryId);
    if(GetWorld() != nullptr){
        FS_InventoryChangedMessage message;
        message.inventoryId = inventoryId;
        UGameplayMessageSubsystem::Get(this).BroadcastMessage(T_Inventory_Changed, message);
    }
}

void UInventoryData::handleReplicatedChange(){
    notifyChanged();
}

// ---------- queries ----------

const FS_InventoryEntry* UInventoryData::findStackable(FName itemId) const{
    const int32 index = findStackableIndex(itemId);
    return index != INDEX_NONE ? &entries.items[index] : nullptr;
}

const FS_InventoryEntry* UInventoryData::findInstance(const FGuid& instanceId) const{
    const int32 index = findInstanceIndex(instanceId);
    return index != INDEX_NONE ? &entries.items[index] : nullptr;
}

const TArray<FS_InventoryEntry>& UInventoryData::getEntries() const{
    return entries.items;
}

TArray<FS_InventoryEntry> UInventoryData::getEntriesCopy() const{
    return entries.items;
}

int32 UInventoryData::getTotalQuantity(FName itemId) const{
    int32 total = 0;
    for(const FS_InventoryEntry& entry : entries.items){
        if(entry.itemId == itemId){
            total += entry.totalQuantity;
        }
    }
    return total;
}

int32 UInventoryData::getReservedQuantity(FName itemId) const{
    int32 reserved = 0;
    for(const FS_InventoryEntry& entry : entries.items){
        if(entry.itemId == itemId){
            reserved += entry.reservedQuantity;
        }
    }
    return reserved;
}

int32 UInventoryData::getAvailableQuantity(FName itemId) const{
    return getTotalQuantity(itemId) - getReservedQuantity(itemId);
}

int32 UInventoryData::getInstanceCount(FName itemId) const{
    int32 count = 0;
    for(const FS_InventoryEntry& entry : entries.items){
        if(entry.isInstanced() && entry.itemId == itemId){
            count++;
        }
    }
    return count;
}

int32 UInventoryData::getAvailableInstanceCount(FName itemId) const{
    int32 count = 0;
    for(const FS_InventoryEntry& entry : entries.items){
        if(entry.isInstanced() && entry.itemId == itemId && entry.reservedQuantity == 0){
            count++;
        }
    }
    return count;
}

bool UInventoryData::isEmpty() const{
    return entries.items.Num() == 0;
}

int32 UInventoryData::getLimitFor(const UDA_ItemDefinition& definition) const{
    if(type != E_InventoryType::Personal && containerLimit > 0){
        return containerLimit;
    }
    return definition.perTypeLimit;
}

int32 UInventoryData::canFit(const FS_ItemStack& stack) const{
    if(stack.quantity <= 0){
        return 0;
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(stack.itemId) : nullptr;
    if(definition == nullptr || definition->isHeavy()){
        return 0;   // a heavy item never enters any inventory (R01 §3.7 step 0)
    }
    const int32 limit = getLimitFor(*definition);
    const int32 current = definition->itemKind == E_ItemKind::Instanced ? getInstanceCount(stack.itemId) : getTotalQuantity(stack.itemId);
    const int32 room = FMath::Max(0, limit - current);
    return FMath::Min(stack.quantity, room);
}

// ---------- commands ----------

FS_InventoryResult UInventoryData::addStack(FName itemId, int32 quantity, int64 spoilAtTime){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    if(quantity <= 0){
        return refuse(T_Inventory_FailReason_InvalidQuantity);
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
    if(definition == nullptr){
        return refuse(T_Inventory_FailReason_UnknownItem);
    }
    if(definition->itemKind != E_ItemKind::Stackable){
        return refuse(T_Inventory_FailReason_WrongItemKind);
    }
    if(definition->isHeavy()){
        return refuse(T_Inventory_FailReason_HeavyItem);
    }
    FS_ItemStack stack;
    stack.itemId = itemId;
    stack.quantity = quantity;
    if(canFit(stack) < quantity){
        return refuse(T_Inventory_FailReason_LimitReached);
    }

    // All checks passed: change state last.
    int64 resolvedSpoilAt = spoilAtTime;
    if(definition->canSpoil && resolvedSpoilAt < 0){
        const UInventoryServices* services = UInventoryServices::get(this);
        const int64 now = services != nullptr ? services->getGameTimeSeconds() : 0;
        resolvedSpoilAt = now + static_cast<int64>(definition->spoilTime * static_cast<float>(ItemInventoryConstants::gameSecondsPerGameHour));
    }
    if(!definition->canSpoil){
        resolvedSpoilAt = -1;
    }

    const int32 index = findStackableIndex(itemId);
    if(index == INDEX_NONE){
        FS_InventoryEntry newEntry;
        newEntry.itemId = itemId;
        newEntry.totalQuantity = quantity;
        newEntry.spoilAtTime = resolvedSpoilAt;
        entries.items.Add(newEntry);
        entries.MarkItemDirty(entries.items.Last());
    }else{
        FS_InventoryEntry& existing = entries.items[index];
        existing.totalQuantity += quantity;
        existing.spoilAtTime = mergeSpoilTime(existing.spoilAtTime, resolvedSpoilAt);
        entries.MarkItemDirty(existing);
    }
    notifyChanged();
    return accept(quantity);
}

FS_InventoryResult UInventoryData::removeStack(FName itemId, int32 quantity){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    if(quantity <= 0){
        return refuse(T_Inventory_FailReason_InvalidQuantity);
    }
    const int32 index = findStackableIndex(itemId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    FS_InventoryEntry& entry = entries.items[index];
    if(quantity > entry.availableQuantity()){
        // Reserved quantity counts as present but is not removable (R01 §3.6).
        return refuse(quantity <= entry.totalQuantity ? T_Inventory_FailReason_ReservedQuantity : T_Inventory_FailReason_InsufficientQuantity);
    }

    entry.totalQuantity -= quantity;
    if(entry.totalQuantity <= 0){
        entries.items.RemoveAt(index);
        entries.MarkArrayDirty();
    }else{
        entries.MarkItemDirty(entry);
    }
    notifyChanged();
    return accept(quantity);
}

FS_InventoryResult UInventoryData::addInstance(const FS_InventoryEntry& instanceEntry){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(instanceEntry.itemId) : nullptr;
    if(definition == nullptr){
        return refuse(T_Inventory_FailReason_UnknownItem);
    }
    if(definition->itemKind != E_ItemKind::Instanced){
        return refuse(T_Inventory_FailReason_WrongItemKind);
    }
    if(definition->isHeavy()){
        return refuse(T_Inventory_FailReason_HeavyItem);
    }
    if(!instanceEntry.instanceId.IsValid()){
        return refuse(T_Inventory_FailReason_InvalidQuantity);
    }
    if(findInstanceIndex(instanceEntry.instanceId) != INDEX_NONE){
        return refuse(T_Inventory_FailReason_InvalidQuantity);   // the same instance cannot exist twice
    }
    FS_ItemStack stack;
    stack.itemId = instanceEntry.itemId;
    stack.quantity = 1;
    if(canFit(stack) < 1){
        return refuse(T_Inventory_FailReason_LimitReached);
    }

    FS_InventoryEntry added = instanceEntry;
    added.ReplicationID = INDEX_NONE;   // never reuse another array's replication ids
    added.totalQuantity = 1;
    added.reservedQuantity = 0;
    entries.items.Add(added);
    entries.MarkItemDirty(entries.items.Last());
    notifyChanged();
    return accept(1);
}

FS_InventoryResult UInventoryData::removeInstance(const FGuid& instanceId){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const int32 index = findInstanceIndex(instanceId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    if(entries.items[index].reservedQuantity > 0){
        return refuse(T_Inventory_FailReason_ReservedQuantity);
    }
    entries.items.RemoveAt(index);
    entries.MarkArrayDirty();
    notifyChanged();
    return accept(1);
}

FS_InventoryResult UInventoryData::setInstanceDurability(const FGuid& instanceId, float newDurability){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const int32 index = findInstanceIndex(instanceId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    FS_InventoryEntry& entry = entries.items[index];
    if(entry.state.durability != ItemInventoryConstants::unbreakableDurability){
        entry.state.durability = FMath::Max(0.0f, newDurability);
        entries.MarkItemDirty(entry);
        notifyChanged();
    }
    return accept(1);
}

FS_InventoryResult UInventoryData::setSpoilAtTime(FName itemId, int64 newSpoilAtTime){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const int32 index = findStackableIndex(itemId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    FS_InventoryEntry& entry = entries.items[index];
    entry.spoilAtTime = newSpoilAtTime;
    entries.MarkItemDirty(entry);
    notifyChanged();
    return accept(1);
}

FS_InventoryResult UInventoryData::replaceEntries(const TArray<FS_InventoryEntry>& newEntries){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    entries.items.Reset();
    for(const FS_InventoryEntry& source : newEntries){
        FS_InventoryEntry copy = source;
        copy.ReplicationID = INDEX_NONE;
        entries.items.Add(copy);
        entries.MarkItemDirty(entries.items.Last());
    }
    entries.MarkArrayDirty();
    notifyChanged();
    return accept(newEntries.Num());
}


FS_InventoryResult UInventoryData::adjustReserved(FName itemId, const FGuid& instanceId, int32 delta){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const int32 index = instanceId.IsValid() ? findInstanceIndex(instanceId) : findStackableIndex(itemId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    FS_InventoryEntry& entry = entries.items[index];
    const int32 newReserved = entry.reservedQuantity + delta;
    if(newReserved < 0 || newReserved > entry.totalQuantity){
        return refuse(delta > 0 ? T_Inventory_FailReason_InsufficientQuantity : T_Inventory_FailReason_InvalidQuantity);
    }
    entry.reservedQuantity = newReserved;
    entries.MarkItemDirty(entry);
    notifyChanged();
    return accept(FMath::Abs(delta));
}

FS_InventoryResult UInventoryData::consumeReserved(FName itemId, const FGuid& instanceId, int32 quantity){
    if(!isServerSide()){
        return refuse(T_Inventory_FailReason_NotServer);
    }
    const int32 index = instanceId.IsValid() ? findInstanceIndex(instanceId) : findStackableIndex(itemId);
    if(index == INDEX_NONE){
        return refuse(T_Inventory_FailReason_NotFound);
    }
    FS_InventoryEntry& entry = entries.items[index];
    if(quantity <= 0 || quantity > entry.reservedQuantity){
        return refuse(T_Inventory_FailReason_InsufficientQuantity);
    }
    entry.reservedQuantity -= quantity;
    entry.totalQuantity -= quantity;
    if(entry.totalQuantity <= 0){
        entries.items.RemoveAt(index);
        entries.MarkArrayDirty();
    }else{
        entries.MarkItemDirty(entry);
    }
    notifyChanged();
    return accept(quantity);
}


FS_SpoilResult UInventoryData::convertStack(FName fromItemId, FName toItemId){
    FS_SpoilResult result;
    if(!isServerSide()){
        return result;
    }
    const int32 fromIndex = findStackableIndex(fromItemId);
    if(fromIndex == INDEX_NONE){
        return result;
    }
    const int32 quantity = entries.items[fromIndex].totalQuantity;
    result.releasedReservedQuantity = entries.items[fromIndex].reservedQuantity;
    result.fromItemId = fromItemId;
    result.toItemId = toItemId;

    entries.items.RemoveAt(fromIndex);
    entries.MarkArrayDirty();

    const int32 toIndex = findStackableIndex(toItemId);
    if(toIndex == INDEX_NONE){
        FS_InventoryEntry spoiled;
        spoiled.itemId = toItemId;
        spoiled.totalQuantity = quantity;
        spoiled.spoilAtTime = -1;   // the spoiled variant does not spoil (validated in the catalog)
        entries.items.Add(spoiled);
        entries.MarkItemDirty(entries.items.Last());
    }else{
        entries.items[toIndex].totalQuantity += quantity;
        entries.MarkItemDirty(entries.items[toIndex]);
    }
    result.converted = true;
    notifyChanged();
    return result;
}
