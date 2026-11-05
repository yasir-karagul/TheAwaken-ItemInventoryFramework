#include "ItemInventoryFramework/UContainerViewComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UContainerViewComponent::UContainerViewComponent(){
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

void UContainerViewComponent::BeginPlay(){
    Super::BeginPlay();
    if(GetOwner() != nullptr && GetOwner()->HasAuthority() && viewMirror == nullptr){
        viewMirror = NewObject<UInventoryData>(this);
        viewMirror->initialize(FGuid::NewGuid(), E_InventoryType::Container);
        AddReplicatedSubObject(viewMirror, COND_OwnerOnly);
    }
}

void UContainerViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason){
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        closeContainerView();   // also releases the viewer record (Session.PlayerLeft cleanup, Section 12)
    }
    Super::EndPlay(EndPlayReason);
}

void UContainerViewComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UContainerViewComponent, viewMirror, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UContainerViewComponent, viewedContainerId, COND_OwnerOnly);
}

UContainerViewComponent* UContainerViewComponent::findFor(const APlayerState* player){
    return player != nullptr ? player->FindComponentByClass<UContainerViewComponent>() : nullptr;
}

UInventoryData* UContainerViewComponent::getViewMirror() const{
    return viewMirror;
}

FGuid UContainerViewComponent::getViewedContainerId() const{
    return viewedContainerId;
}

FS_InventoryResult UContainerViewComponent::openContainerView(const FGuid& containerId){
    FS_InventoryResult result;
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority() || viewMirror == nullptr){
        result.failReason = T_Inventory_FailReason_NotServer;
        return result;
    }
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    UInventoryData* source = registry != nullptr ? registry->findInventory(containerId) : nullptr;
    if(source == nullptr){
        result.failReason = T_Inventory_FailReason_NotFound;
        return result;
    }
    if(source->isDeathBag() || source->getType() == E_InventoryType::Personal){
        result.failReason = T_Inventory_FailReason_DeathBagLocked;   // a bag is not a container (R01 §3.4)
        return result;
    }
    // A Bagless player cannot use container storage (R01 §3.8 rule 5).
    const UPlayerInventoryComponent* playerInventory = GetOwner()->FindComponentByClass<UPlayerInventoryComponent>();
    if(playerInventory == nullptr || !playerInventory->hasBag()){
        result.failReason = T_Inventory_FailReason_NoBag;
        return result;
    }

    // Everything checked: change state last.
    closeContainerView();
    viewedSource = source;
    viewedContainerId = containerId;
    source->onInventoryChanged.AddDynamic(this, &UContainerViewComponent::handleSourceChanged);
    registry->addViewer(containerId, this);
    UBFL_InventoryConsume::evaluateAllSpoilage(source);
    viewMirror->replaceEntries(source->getEntries());
    result.quantity = 1;
    return result;
}

void UContainerViewComponent::closeContainerView(){
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    UInventoryData* source = viewedSource.Get();
    if(source != nullptr){
        source->onInventoryChanged.RemoveDynamic(this, &UContainerViewComponent::handleSourceChanged);
    }
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    if(registry != nullptr && viewedContainerId.IsValid()){
        registry->removeViewer(viewedContainerId, this);
    }
    viewedSource.Reset();
    viewedContainerId.Invalidate();
    if(viewMirror != nullptr){
        viewMirror->replaceEntries(TArray<FS_InventoryEntry>());
    }
}

void UContainerViewComponent::handleSourceChanged(const FGuid& changedId){
    const UInventoryData* source = viewedSource.Get();
    if(source != nullptr && viewMirror != nullptr){
        viewMirror->replaceEntries(source->getEntries());
    }
}
