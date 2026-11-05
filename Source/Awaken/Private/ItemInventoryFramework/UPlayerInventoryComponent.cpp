#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UPlayerInventoryComponent::UPlayerInventoryComponent(){
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

void UPlayerInventoryComponent::BeginPlay(){
    Super::BeginPlay();
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        createPersonalInventory();
    }
}

void UPlayerInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UPlayerInventoryComponent, personalInventory, COND_OwnerOnly);
}

UPlayerInventoryComponent* UPlayerInventoryComponent::findFor(const APlayerState* player){
    return player != nullptr ? player->FindComponentByClass<UPlayerInventoryComponent>() : nullptr;
}

UInventoryData* UPlayerInventoryComponent::getPersonalInventory() const{
    return personalInventory;
}

void UPlayerInventoryComponent::createPersonalInventory(){
    if(personalInventory != nullptr){
        return;
    }
    personalInventory = NewObject<UInventoryData>(this);
    personalInventory->initialize(FGuid::NewGuid(), E_InventoryType::Personal);
    AddReplicatedSubObject(personalInventory, COND_OwnerOnly);
    INVENTORY_LOG(Log, this, "Personal inventory %s created for %s", *personalInventory->getInventoryId().ToString(), *GetOwner()->GetName());
}

void UPlayerInventoryComponent::OnRep_personalInventory(){
    INVENTORY_LOG(Verbose, this, "Personal inventory replicated");
}


void UPlayerInventoryComponent::setCharacterId(const FGuid& newCharacterId){
    characterId = newCharacterId;
}

FGuid UPlayerInventoryComponent::getCharacterId() const{
    return characterId;
}

bool UPlayerInventoryComponent::isStartingKitGranted() const{
    return startingKitGranted;
}

void UPlayerInventoryComponent::markStartingKitGranted(){
    startingKitGranted = true;
}

UInventoryData* UPlayerInventoryComponent::detachPersonalInventory(UObject* newOuter){
    UInventoryData* handedOut = personalInventory;
    if(handedOut == nullptr || GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return nullptr;
    }
    handedOut->onInventoryChanged.RemoveDynamic(this, &UPlayerInventoryComponent::handlePersonalInventoryChanged);
    RemoveReplicatedSubObject(handedOut);
    personalInventory = nullptr;
    handedOut->Rename(nullptr, newOuter, REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
    revalidateHeldItem();
    return handedOut;
}

void UPlayerInventoryComponent::attachPersonalInventory(UInventoryData* inventory){
    if(inventory == nullptr || personalInventory != nullptr || GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    inventory->Rename(nullptr, this, REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
    personalInventory = inventory;
    AddReplicatedSubObject(personalInventory, COND_OwnerOnly);
    bindPersonalInventory();
}

void UPlayerInventoryComponent::resetForNewCharacter(const FGuid& newCharacterId){
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    releaseAllReservations();
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    UInventoryData* old = detachPersonalInventory(registry);
    if(old != nullptr && registry != nullptr){
        registry->destroyInventory(old->getInventoryId());   // the old character's bag content is gone for good
    }
    createPersonalInventory();
    characterId = newCharacterId;
    startingKitGranted = false;
}
