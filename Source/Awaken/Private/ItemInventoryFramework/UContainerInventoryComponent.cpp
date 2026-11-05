#include "ItemInventoryFramework/UContainerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/InventoryLog.h"

void UContainerInventoryComponent::OnComponentCreated(){
    Super::OnComponentCreated();
    if(!containerId.IsValid() && !HasAnyFlags(RF_ClassDefaultObject)){
        containerId = FGuid::NewGuid();
    }
}

void UContainerInventoryComponent::BeginPlay(){
    Super::BeginPlay();
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    if(registry == nullptr || GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    if(!containerId.IsValid()){
        containerId = FGuid::NewGuid();   // player-built; the building record must store it
    }
    UInventoryData* inventory = registry->findInventory(containerId);
    if(inventory == nullptr){
        inventory = registry->createInventory(E_InventoryType::Container, containerId);
    }
    if(inventory != nullptr){
        inventory->setContainerRules(perTypeLimit, allowedTags);
    }
    registry->registerContainerComponent(containerId, this);
}

void UContainerInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason){
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    if(registry != nullptr){
        registry->unregisterContainerComponent(containerId);   // the DATA stays (Doc02 §18.1)
    }
    Super::EndPlay(EndPlayReason);
}

UInventoryData* UContainerInventoryComponent::getInventory() const{
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    return registry != nullptr ? registry->findInventory(containerId) : nullptr;
}
