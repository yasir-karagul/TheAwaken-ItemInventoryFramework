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
