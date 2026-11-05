#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UHeldItemComponent::UHeldItemComponent(){
    SetIsReplicatedByDefault(true);
}

void UHeldItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UHeldItemComponent, heldItemId);
    DOREPLIFETIME(UHeldItemComponent, heldInstanceId);
    DOREPLIFETIME(UHeldItemComponent, displayItemId);
}

UHeldItemComponent* UHeldItemComponent::findFor(const APawn* pawn){
    return pawn != nullptr ? pawn->FindComponentByClass<UHeldItemComponent>() : nullptr;
}

FS_HeldItemView UHeldItemComponent::getHeldView() const{
    FS_HeldItemView view;
    if(heldItemId.IsNone()){
        view.itemId = UItemInventorySettings::get()->unarmedItemId;
        view.isUnarmed = true;
        return view;
    }
    view.itemId = heldItemId;
    view.instanceId = heldInstanceId;
    view.isUnarmed = false;
    return view;
}

FName UHeldItemComponent::getDisplayItemId() const{
    return displayItemId;
}

bool UHeldItemComponent::isInstanceHeld(const FGuid& instanceId) const{
    return instanceId.IsValid() && heldInstanceId == instanceId;
}

void UHeldItemComponent::setWielded(FName itemId, const FGuid& instanceId){
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        heldItemId = itemId;
        heldInstanceId = instanceId;
        displayItemId = NAME_None;
    }
}

void UHeldItemComponent::setDisplayOnly(FName itemId){
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        displayItemId = itemId;
    }
}

void UHeldItemComponent::clearToUnarmed(){
    if(GetOwner() != nullptr && GetOwner()->HasAuthority()){
        heldItemId = NAME_None;
        heldInstanceId.Invalidate();
        displayItemId = NAME_None;
    }
}

void UHeldItemComponent::revalidate(){
    if(GetOwner() == nullptr || !GetOwner()->HasAuthority()){
        return;
    }
    const APawn* pawn = Cast<APawn>(GetOwner());
    const APlayerState* player = pawn != nullptr ? pawn->GetPlayerState() : nullptr;
    const UPlayerInventoryComponent* inventory = UPlayerInventoryComponent::findFor(player);
    const UInventoryData* bag = inventory != nullptr ? inventory->getPersonalInventory() : nullptr;

    if(!heldItemId.IsNone()){
        bool stillThere = false;
        if(bag != nullptr){
            if(heldInstanceId.IsValid()){
                stillThere = bag->findInstance(heldInstanceId) != nullptr;
            }else{
                stillThere = bag->getAvailableQuantity(heldItemId) > 0;
            }
        }
        if(!stillThere){
            clearToUnarmed();   // broken, thrown, moved, or the bag left the player
            return;
        }
    }
    if(!displayItemId.IsNone()){
        const UHotbarComponent* hotbar = UHotbarComponent::findFor(player);
        if(hotbar == nullptr || hotbar->findHoldSlotIndex(displayItemId, FGuid()) == INDEX_NONE){
            displayItemId = NAME_None;
        }
    }
}
