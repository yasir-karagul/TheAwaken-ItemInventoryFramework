#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "UInventoryRegistrySubsystem.generated.h"

class UInventoryData;
class UContainerInventoryComponent;
class UContainerViewComponent;

UCLASS()
class AWAKEN_API UInventoryRegistrySubsystem : public UWorldSubsystem{
    GENERATED_BODY()

public:
    static UInventoryRegistrySubsystem* get(const UObject* worldContext);

    // Command, server only. Creates an inventory owned (as UObject outer) by this subsystem and registers it.
    UInventoryData* createInventory(E_InventoryType type, const FGuid& forcedId = FGuid());
    // Command, server only. Registers an inventory created elsewhere (the Personal Inventory lives under its component).
    void registerInventory(UInventoryData* inventory);
    // Query.
    UInventoryData* findInventory(const FGuid& inventoryId) const;
    // Command, server only. Drops the registry reference; the object is garbage collected when unreferenced.
    bool destroyInventory(const FGuid& inventoryId);
    const TMap<FGuid, TObjectPtr<UInventoryData>>& getAll() const;

    // Containers: the actor is only the loaded view of the data (Doc02 §18.1).
    void registerContainerComponent(const FGuid& containerId, UContainerInventoryComponent* component);
    void unregisterContainerComponent(const FGuid& containerId);
    UContainerInventoryComponent* findContainerComponent(const FGuid& containerId) const;

    // S4.8: who has a container open.
    void addViewer(const FGuid& containerId, UContainerViewComponent* viewer);
    void removeViewer(const FGuid& containerId, UContainerViewComponent* viewer);
    int32 getViewerCount(const FGuid& containerId) const;

private:
    UPROPERTY() TMap<FGuid, TObjectPtr<UInventoryData>> inventories;
    TMap<FGuid, TWeakObjectPtr<UContainerInventoryComponent>> containerComponents;
    TMap<FGuid, TArray<TWeakObjectPtr<UContainerViewComponent>>> viewers;
};
