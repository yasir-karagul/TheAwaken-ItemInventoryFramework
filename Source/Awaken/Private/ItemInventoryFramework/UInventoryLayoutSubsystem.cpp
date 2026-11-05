#include "ItemInventoryFramework/UInventoryLayoutSubsystem.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UDA_InventoryPageLayout.h"
#include "ItemInventoryFramework/UDA_InventoryCategoryMap.h"
#include "ItemInventoryFramework/UDA_StackLayout.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

void UInventoryLayoutSubsystem::Initialize(FSubsystemCollectionBase& Collection){
    Collection.InitializeDependency<UItemCatalogSubsystem>();
    Super::Initialize(Collection);
    rebuild();
    TArray<FString> errors;
    if(!validateLayout(errors)){
        for(const FString& error : errors){
            INVENTORY_LOG(Error, this, "Layout: %s", *error);   // S10.6: an item without a slot is logged, never invented
        }
    }
}

UInventoryLayoutSubsystem* UInventoryLayoutSubsystem::get(const UObject* worldContext){
    UGameInstance* gameInstance = UGameplayStatics::GetGameInstance(worldContext);
    return gameInstance != nullptr ? gameInstance->GetSubsystem<UInventoryLayoutSubsystem>() : nullptr;
}

void UInventoryLayoutSubsystem::rebuild(){
    itemToPage.Reset();
    itemToSlot.Reset();
    slotToItems.Reset();
    layouts.Reset();
    categoryMap = nullptr;

    const UItemInventorySettings* settings = UItemInventorySettings::get();
    categoryMap = settings->categoryMap.LoadSynchronous();
    for(const TPair<E_InventoryPage, TSoftObjectPtr<UDA_InventoryPageLayout>>& pair : settings->pageLayouts){
        layouts.Add(pair.Key, pair.Value.LoadSynchronous());
    }

    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    if(catalog == nullptr){
        return;
    }
    for(const TPair<FName, TObjectPtr<UDA_ItemDefinition>>& pair : catalog->getAllDefinitions()){
        const UDA_ItemDefinition* definition = pair.Value;
        if(definition == nullptr || definition->isHeavy() || definition->itemId == settings->unarmedItemId){
            continue;   // never in the bag, so no slot
        }
        const E_InventoryPage page = categoryMap != nullptr ? categoryMap->getPageFor(definition->categoryTag) : E_InventoryPage::Others;
        itemToPage.Add(definition->itemId, page);
        if(!definition->inventorySlot.IsNone()){
            itemToSlot.Add(definition->itemId, definition->inventorySlot);
            slotToItems.FindOrAdd(page).FindOrAdd(definition->inventorySlot).AddUnique(definition->itemId);
        }
    }
}

E_InventoryPage UInventoryLayoutSubsystem::getPageForItem(FName itemId) const{
    const E_InventoryPage* page = itemToPage.Find(itemId);
    return page != nullptr ? *page : E_InventoryPage::Others;
}

FName UInventoryLayoutSubsystem::getSlotForItem(FName itemId) const{
    const FName* slot = itemToSlot.Find(itemId);
    return slot != nullptr ? *slot : NAME_None;
}

TArray<FS_InventorySlotDef> UInventoryLayoutSubsystem::getSlotsForPage(E_InventoryPage page) const{
    const TObjectPtr<UDA_InventoryPageLayout>* layout = layouts.Find(page);
    if(layout == nullptr || layout->Get() == nullptr){
        return TArray<FS_InventorySlotDef>();
    }
    return (*layout)->slots;
}

TArray<FS_SlotView> UInventoryLayoutSubsystem::getSlotView(APlayerState* player, E_InventoryPage page) const{
    TArray<FS_SlotView> views;
    const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UInventoryData* bag = component != nullptr ? component->getPersonalInventory() : nullptr;
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const TMap<FName, TArray<FName>>* pageSlots = slotToItems.Find(page);

    auto quantityOf = [bag, catalog](FName itemId) -> int32{
        const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
        if(bag == nullptr || definition == nullptr){
            return 0;
        }
        // Available quantity (held minus reserved), so a staged item shows less (R01 §3.6; Index A115 S12-P8).
        return definition->itemKind == E_ItemKind::Instanced ? bag->getAvailableInstanceCount(itemId) : bag->getAvailableQuantity(itemId);
    };

    for(const FS_InventorySlotDef& slotDef : getSlotsForPage(page)){
        FS_SlotView view;
        view.slotId = slotDef.slotId;
        const TArray<FName>* owners = pageSlots != nullptr ? pageSlots->Find(slotDef.slotId) : nullptr;
        if(owners != nullptr && owners->Num() > 0){
            view.itemId = (*owners)[0];
            view.quantity = quantityOf(view.itemId);
            if(owners->Num() > 1){
                view.isVariantPair = true;
                view.variantItemId = (*owners)[1];
                view.variantQuantity = quantityOf(view.variantItemId);
            }
        }
        views.Add(view);   // an empty slot is always drawn (R01 §3.1a)
    }
    return views;
}

TArray<FS_InstanceCopyView> UInventoryLayoutSubsystem::getInstanceCopies(APlayerState* player, FName itemId) const{
    TArray<FS_InstanceCopyView> copies;
    const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
    const UInventoryData* bag = component != nullptr ? component->getPersonalInventory() : nullptr;
    if(bag == nullptr){
        return copies;
    }
    const FGuid nextHotbar = hotbar != nullptr ? hotbar->getNextForHotbar(itemId) : FGuid();
    const FGuid nextCrafting = component->findMostWornAvailableInstance(itemId);
    for(const FS_InventoryEntry& entry : bag->getEntries()){
        if(entry.isInstanced() && entry.itemId == itemId){
            FS_InstanceCopyView copy;
            copy.instanceId = entry.instanceId;
            copy.durability = entry.state.durability;
            copy.isNextForHotbar = entry.instanceId == nextHotbar;
            copy.isNextForCrafting = entry.instanceId == nextCrafting;
            copies.Add(copy);
        }
    }
    return copies;
}

bool UInventoryLayoutSubsystem::validateLayout(TArray<FString>& errors) const{
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    if(catalog == nullptr){
        return true;
    }
    // Slot ids unique inside each layout (R01 §3.1b).
    for(const TPair<E_InventoryPage, TObjectPtr<UDA_InventoryPageLayout>>& pair : layouts){
        TSet<FName> seen;
        if(pair.Value == nullptr){
            continue;
        }
        for(const FS_InventorySlotDef& slotDef : pair.Value->slots){
            if(slotDef.slotId.IsNone() || seen.Contains(slotDef.slotId)){
                errors.Add(FString::Printf(TEXT("Page %d has an empty or duplicate slotId %s"), static_cast<int32>(pair.Key), *slotDef.slotId.ToString()));
            }
            seen.Add(slotDef.slotId);
        }
    }
    for(const TPair<FName, E_InventoryPage>& pair : itemToPage){
        const UDA_ItemDefinition* definition = catalog->getDefinition(pair.Key);
        if(definition == nullptr){
            continue;
        }
        const FString id = pair.Key.ToString();
        const TObjectPtr<UDA_InventoryPageLayout>* layout = layouts.Find(pair.Value);
        if(definition->inventorySlot.IsNone()){
            errors.Add(FString::Printf(TEXT("%s has no inventorySlot"), *id));
        }else if(layout == nullptr || layout->Get() == nullptr){
            errors.Add(FString::Printf(TEXT("%s maps to page %d which has no layout"), *id, static_cast<int32>(pair.Value)));
        }else if((*layout)->findSlot(definition->inventorySlot) == nullptr){
            // This is also what catches a changed categoryTag whose slot is still on the old page (R01 §3.1a).
            errors.Add(FString::Printf(TEXT("%s: slot %s does not exist on its page %d"), *id, *definition->inventorySlot.ToString(), static_cast<int32>(pair.Value)));
        }
        if(definition->inventoryMesh.IsNull()){
            errors.Add(FString::Printf(TEXT("%s has no inventoryMesh"), *id));
        }
        if(definition->stackLayout.IsNull()){
            errors.Add(FString::Printf(TEXT("%s has no stackLayout"), *id));
        }else{
            const UDA_StackLayout* stack = definition->stackLayout.LoadSynchronous();
            if(stack == nullptr || stack->units.Num() == 0){
                errors.Add(FString::Printf(TEXT("%s: stack layout is empty"), *id));
            }else{
                for(const FS_StackUnitPlacement& unit : stack->units){
                    if(FMath::Abs(unit.location.X) > stack->pocketHalfExtent.X || FMath::Abs(unit.location.Y) > stack->pocketHalfExtent.Y || FMath::Abs(unit.location.Z) > stack->pocketHalfExtent.Z){
                        errors.Add(FString::Printf(TEXT("%s: a full pile does not fit its pocket"), *id));
                        break;
                    }
                }
            }
        }
    }
    // No two items share a slot unless they are declared variants (R01 §3.1a).
    for(const TPair<E_InventoryPage, TMap<FName, TArray<FName>>>& pagePair : slotToItems){
        for(const TPair<FName, TArray<FName>>& slotPair : pagePair.Value){
            const TArray<FName>& owners = slotPair.Value;
            if(owners.Num() > 2){
                errors.Add(FString::Printf(TEXT("Slot %s is shared by %d items; at most a declared pair"), *slotPair.Key.ToString(), owners.Num()));
            }else if(owners.Num() == 2){
                const UDA_ItemDefinition* first = catalog->getDefinition(owners[0]);
                const UDA_ItemDefinition* second = catalog->getDefinition(owners[1]);
                if(first == nullptr || second == nullptr || first->slotSharedWith != second->itemId || second->slotSharedWith != first->itemId){
                    errors.Add(FString::Printf(TEXT("Slot %s is shared by %s and %s without a declared variant link"), *slotPair.Key.ToString(), *owners[0].ToString(), *owners[1].ToString()));
                }
            }
        }
    }
    return errors.Num() == 0;
}
