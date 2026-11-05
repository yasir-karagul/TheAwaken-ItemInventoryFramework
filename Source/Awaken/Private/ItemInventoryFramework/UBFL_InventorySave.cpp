#include "ItemInventoryFramework/UBFL_InventorySave.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UBFL_InventoryConsume.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

FS_InventorySaveRecord UBFL_InventorySave::makeRecord(UInventoryData* inventory, bool useTimeLeft){
    FS_InventorySaveRecord record;
    record.inventoryId = inventory->getInventoryId();
    record.type = inventory->getType();
    record.isDeathBag = inventory->isDeathBag();
    record.containerLimit = inventory->getContainerLimit();
    for(const FGameplayTag& tag : inventory->getAllowedTags()){
        record.allowedTags.Add(tag);
    }
    const UInventoryServices* services = UInventoryServices::get(inventory);
    const int64 now = services != nullptr ? services->getGameTimeSeconds() : 0;
    for(const FS_InventoryEntry& entry : inventory->getEntries()){
        FS_SavedInventoryEntry saved;
        saved.itemId = entry.itemId;
        saved.totalQuantity = entry.totalQuantity;   // reservedQuantity is NOT saved (R01 §8)
        saved.instanceId = entry.instanceId;
        saved.state = entry.state;
        if(entry.spoilAtTime >= 0){
            saved.spoilValue = useTimeLeft ? FMath::Max<int64>(0, entry.spoilAtTime - now) : entry.spoilAtTime;
        }
        record.entries.Add(saved);
    }
    return record;
}

FS_InventorySaveBundle UBFL_InventorySave::captureBundle(UInventoryData* inventory, bool useTimeLeft){
    FS_InventorySaveBundle bundle;
    if(inventory == nullptr){
        return bundle;
    }
    UBFL_InventoryConsume::evaluateAllSpoilage(inventory);   // S8.4: lazy evaluation before capture
    bundle.records.Add(makeRecord(inventory, useTimeLeft));
    // A Portable Container's contents are saved once, together with its holder (R01 §3.4).
    const UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(inventory);
    for(const FS_InventoryEntry& entry : inventory->getEntries()){
        if(entry.isInstanced() && entry.state.containedInventoryId.IsValid() && registry != nullptr){
            UInventoryData* nested = registry->findInventory(entry.state.containedInventoryId);
            if(nested != nullptr){
                UBFL_InventoryConsume::evaluateAllSpoilage(nested);
                bundle.records.Add(makeRecord(nested, useTimeLeft));
            }
        }
    }
    return bundle;
}

void UBFL_InventorySave::applyBundle(UInventoryData* target, const FS_InventorySaveBundle& bundle, bool useTimeLeft){
    if(target == nullptr || bundle.records.Num() == 0){
        return;
    }
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(target);
    const UInventoryServices* services = UInventoryServices::get(target);
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(target);
    const int64 now = services != nullptr ? services->getGameTimeSeconds() : 0;

    auto toEntries = [&](const FS_InventorySaveRecord& record){
        TArray<FS_InventoryEntry> entries;
        for(const FS_SavedInventoryEntry& saved : record.entries){
            const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(saved.itemId) : nullptr;
            if(definition == nullptr){
                // R13 §3.11 ST-13: skip the entry, one log line, the load continues. Never crash.
                INVENTORY_LOG(Warning, target, "Skipped saved item %s: the item id no longer exists", *saved.itemId.ToString());
                continue;
            }
            FS_InventoryEntry entry;
            entry.itemId = saved.itemId;
            entry.totalQuantity = saved.totalQuantity;
            entry.reservedQuantity = 0;
            entry.instanceId = saved.instanceId;
            entry.state = saved.state;
            entry.spoilAtTime = saved.spoilValue < 0 ? -1 : (useTimeLeft ? now + saved.spoilValue : saved.spoilValue);
            entries.Add(entry);
        }
        return entries;
    };

    const FS_InventorySaveRecord& main = bundle.records[0];
    target->setContainerRules(main.containerLimit, FGameplayTagContainer::CreateFromArray(main.allowedTags));
    target->setDeathBag(main.isDeathBag);
    target->replaceEntries(toEntries(main));

    // Nested Portable Container inventories keep their saved ids so containedInventoryId still resolves.
    for(int32 index = 1; index < bundle.records.Num(); index++){
        const FS_InventorySaveRecord& nestedRecord = bundle.records[index];
        UInventoryData* nested = registry != nullptr ? registry->createInventory(nestedRecord.type, nestedRecord.inventoryId) : nullptr;
        if(nested != nullptr){
            nested->setContainerRules(nestedRecord.containerLimit, FGameplayTagContainer::CreateFromArray(nestedRecord.allowedTags));
            nested->replaceEntries(toEntries(nestedRecord));
        }
    }
}

TArray<uint8> UBFL_InventorySave::toBytes(UScriptStruct* structType, const void* structData){
    TArray<uint8> bytes;
    FMemoryWriter memoryWriter(bytes, true);
    FObjectAndNameAsStringProxyArchive archive(memoryWriter, false);
    archive.ArIsSaveGame = true;
    structType->SerializeBin(archive, const_cast<void*>(structData));
    return bytes;
}

bool UBFL_InventorySave::fromBytes(UScriptStruct* structType, void* structData, const TArray<uint8>& bytes){
    if(bytes.Num() == 0){
        return false;
    }
    FMemoryReader memoryReader(bytes, true);
    FObjectAndNameAsStringProxyArchive archive(memoryReader, true);
    archive.ArIsSaveGame = true;
    structType->SerializeBin(archive, structData);
    return !archive.IsError();
}

bool UBFL_InventorySave::migrateCharacterSave(FS_CharacterInventorySave& record){
    // One step per save key, chained from the file's version to the current one (R13 §3.11, S1-F30).
    while(record.version < currentSnapshotVersion){
        switch(record.version){
            // case 0:{ ...upgrade 0 -> 1...; record.version = 1; break; }
            default:{
                return false;   // a missing step: the file is refused
            }
        }
    }
    return record.version == currentSnapshotVersion;   // a NEWER file is refused as well
}

bool UBFL_InventorySave::migrateWorldSave(FS_WorldInventorySave& record){
    while(record.version < currentSnapshotVersion){
        switch(record.version){
            default:{
                return false;
            }
        }
    }
    return record.version == currentSnapshotVersion;
}
