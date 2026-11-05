#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UContainerInventoryComponent.h"
#include "ItemInventoryFramework/UContainerViewComponent.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UInventoryRegistrySubsystem* UInventoryRegistrySubsystem::get(const UObject* worldContext){
    if(GEngine == nullptr || worldContext == nullptr){
        return nullptr;
    }
    UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
    return world != nullptr ? world->GetSubsystem<UInventoryRegistrySubsystem>() : nullptr;
}

UInventoryData* UInventoryRegistrySubsystem::createInventory(E_InventoryType type, const FGuid& forcedId){
    const UWorld* world = GetWorld();
    if(world == nullptr || world->GetNetMode() == NM_Client){
        return nullptr;
    }
    const FGuid id = forcedId.IsValid() ? forcedId : FGuid::NewGuid();
    if(inventories.Contains(id)){
        return inventories[id];
    }
    UInventoryData* inventory = NewObject<UInventoryData>(this);
    inventory->initialize(id, type);
    inventories.Add(id, inventory);
    INVENTORY_LOG(Log, this, "Inventory %s created (%d)", *id.ToString(), static_cast<int32>(type));
    return inventory;
}

void UInventoryRegistrySubsystem::registerInventory(UInventoryData* inventory){
    if(inventory != nullptr && inventory->getInventoryId().IsValid()){
        inventories.Add(inventory->getInventoryId(), inventory);
    }
}

UInventoryData* UInventoryRegistrySubsystem::findInventory(const FGuid& inventoryId) const{
    const TObjectPtr<UInventoryData>* found = inventories.Find(inventoryId);
    return found != nullptr ? found->Get() : nullptr;
}

bool UInventoryRegistrySubsystem::destroyInventory(const FGuid& inventoryId){
    return inventories.Remove(inventoryId) > 0;
}

const TMap<FGuid, TObjectPtr<UInventoryData>>& UInventoryRegistrySubsystem::getAll() const{
    return inventories;
}

void UInventoryRegistrySubsystem::registerContainerComponent(const FGuid& containerId, UContainerInventoryComponent* component){
    containerComponents.Add(containerId, component);
}

void UInventoryRegistrySubsystem::unregisterContainerComponent(const FGuid& containerId){
    containerComponents.Remove(containerId);
}

UContainerInventoryComponent* UInventoryRegistrySubsystem::findContainerComponent(const FGuid& containerId) const{
    const TWeakObjectPtr<UContainerInventoryComponent>* found = containerComponents.Find(containerId);
    return found != nullptr ? found->Get() : nullptr;
}

void UInventoryRegistrySubsystem::addViewer(const FGuid& containerId, UContainerViewComponent* viewer){
    TArray<TWeakObjectPtr<UContainerViewComponent>>& list = viewers.FindOrAdd(containerId);
    list.AddUnique(viewer);
}

void UInventoryRegistrySubsystem::removeViewer(const FGuid& containerId, UContainerViewComponent* viewer){
    TArray<TWeakObjectPtr<UContainerViewComponent>>* list = viewers.Find(containerId);
    if(list != nullptr){
        list->Remove(viewer);
        if(list->Num() == 0){
            viewers.Remove(containerId);
        }
    }
}

int32 UInventoryRegistrySubsystem::getViewerCount(const FGuid& containerId) const{
    const TArray<TWeakObjectPtr<UContainerViewComponent>>* list = viewers.Find(containerId);
    return list != nullptr ? list->Num() : 0;
}
