#include "ItemInventoryFramework/UInventoryRequestComponent.h"
#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/UBFL_InventoryConsume.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UContainerViewComponent.h"
#include "ItemInventoryFramework/UContainerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/ALooseItemActor.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"

UInventoryRequestComponent* UInventoryRequestComponent::findFor(const APlayerState* player){
    const APlayerController* controller = player != nullptr ? Cast<APlayerController>(player->GetOwner()) : nullptr;
    return controller != nullptr ? controller->FindComponentByClass<UInventoryRequestComponent>() : nullptr;
}

APlayerState* UInventoryRequestComponent::getPlayerState() const{
    const APlayerController* controller = Cast<APlayerController>(GetOwner());
    return controller != nullptr ? controller->PlayerState.Get() : nullptr;
}

bool UInventoryRequestComponent::isValidQuantity(int32 quantity) const{
    return quantity > 0 && quantity <= UItemInventorySettings::get()->maxRequestQuantity;
}

void UInventoryRequestComponent::refuse(const FGameplayTag& reason, const TCHAR* operation){
    // Every refusal logs its reason tag, so a test (and a human) can read it (Doc B22 3.6).
    INVENTORY_LOG(Warning, this, "%s refused: %s", operation, *reason.ToString());
    clientRefused(reason);
}

FGameplayTag UInventoryRequestComponent::gate(bool needsHands, bool allowWhenIncapacitated){
    const UItemInventorySettings* settings = UItemInventorySettings::get();
    const double now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
    if(!bucketReady){
        tokens = settings->requestsPerSecond;
        lastRefillTime = now;
        bucketReady = true;
    }
    tokens = FMath::Min(settings->requestsPerSecond, tokens + static_cast<float>(now - lastRefillTime) * settings->requestsPerSecond);
    lastRefillTime = now;
    if(tokens < 1.0f){
        return T_Inventory_FailReason_RateLimited;   // S12.7: a client cannot flood the host
    }
    tokens -= 1.0f;

    APlayerState* player = getPlayerState();
    const UInventoryServices* services = UInventoryServices::get(this);
    if(player == nullptr || services == nullptr){
        return T_Inventory_FailReason_NotFound;
    }
    if(!allowWhenIncapacitated && services->isIncapacitated(player)){
        return T_Interaction_FailReason_NotAllowedInState;
    }
    if(needsHands && services->isCarryActive(player)){
        return T_Interaction_FailReason_HandsFull;   // Doc B07 3.6: bag screen, equipping and eating are refused while carrying
    }
    return FGameplayTag();
}

bool UInventoryRequestComponent::isContainerReachable(APlayerState* player, const FGuid& containerId) const{
    const UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    const UInventoryServices* services = UInventoryServices::get(this);
    if(registry == nullptr || services == nullptr || player == nullptr){
        return false;
    }
    const UContainerInventoryComponent* container = registry->findContainerComponent(containerId);
    if(container != nullptr && container->GetOwner() != nullptr){
        const AActor* target = container->GetOwner();
        const APawn* pawn = player->GetPawn();
        const float range = UItemInventorySettings::get()->containerRange;
        const bool withinDistance = pawn != nullptr && FVector::Dist(pawn->GetActorLocation(), target->GetActorLocation()) <= range;   // inclusive (Doc02 §19.9)
        return withinDistance && services->isInInteractRange(player, const_cast<AActor*>(target));
    }
    // A Portable Container the player holds in the bag needs no range.
    const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UInventoryData* bag = component != nullptr ? component->getPersonalInventory() : nullptr;
    if(bag != nullptr){
        for(const FS_InventoryEntry& entry : bag->getEntries()){
            if(entry.isInstanced() && entry.state.containedInventoryId == containerId){
                return true;
            }
        }
    }
    return false;
}

void UInventoryRequestComponent::serverOpenContainer_Implementation(FGuid containerId){
    const FGameplayTag blocked = gate(true, false);
    if(blocked.IsValid()){
        refuse(blocked, TEXT("openContainer"));
        return;
    }
    APlayerState* player = getPlayerState();
    UContainerViewComponent* view = UContainerViewComponent::findFor(player);
    if(view == nullptr){
        refuse(T_Inventory_FailReason_NotFound, TEXT("openContainer"));
        return;
    }
    if(!isContainerReachable(player, containerId)){
        refuse(T_Inventory_FailReason_OutOfRange, TEXT("openContainer"));
        return;
    }
    const FS_InventoryResult opened = view->openContainerView(containerId);
    if(!opened.succeeded()){
        refuse(opened.failReason, TEXT("openContainer"));
    }
}

void UInventoryRequestComponent::serverCloseContainer_Implementation(){
    UContainerViewComponent* view = UContainerViewComponent::findFor(getPlayerState());
    if(view != nullptr){
        view->closeContainerView();   // closing is always allowed and costs no token
    }
}

void UInventoryRequestComponent::serverTransfer_Implementation(bool toContainer, FName itemId, FGuid instanceId, int32 quantity){
    const FGameplayTag blocked = gate(true, false);
    if(blocked.IsValid() || !isValidQuantity(quantity)){
        refuse(blocked.IsValid() ? blocked : T_Inventory_FailReason_InvalidQuantity, TEXT("transfer"));
        return;
    }
    APlayerState* player = getPlayerState();
    UContainerViewComponent* view = UContainerViewComponent::findFor(player);
    const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
    const UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    const FGuid viewedId = view != nullptr ? view->getViewedContainerId() : FGuid();
    if(!viewedId.IsValid() || component == nullptr || registry == nullptr || !component->hasBag()){
        refuse(T_Inventory_FailReason_NotFound, TEXT("transfer"));
        return;
    }
    if(!isContainerReachable(player, viewedId)){
        view->closeContainerView();   // the player walked away: close the view and refuse
        refuse(T_Inventory_FailReason_OutOfRange, TEXT("transfer"));
        return;
    }
    UInventoryData* bag = component->getPersonalInventory();
    UInventoryData* container = registry->findInventory(viewedId);   // the REAL container, never the mirror
    const FS_InventoryResult moved = toContainer
        ? UBFL_Inventory::transferEntry(bag, container, itemId, instanceId, quantity)
        : UBFL_Inventory::transferEntry(container, bag, itemId, instanceId, quantity);
    if(!moved.succeeded()){
        refuse(moved.failReason, TEXT("transfer"));
    }
}

void UInventoryRequestComponent::serverBindHotbar_Implementation(int32 slotIndex, FName itemId){
    const FGameplayTag blocked = gate(true, false);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(getPlayerState());
    if(blocked.IsValid() || hotbar == nullptr){
        refuse(blocked.IsValid() ? blocked : T_Inventory_FailReason_NotFound, TEXT("bindHotbar"));
        return;
    }
    const FS_InventoryResult bound = hotbar->bindHotbarSlot(slotIndex, itemId);
    if(!bound.succeeded()){
        refuse(bound.failReason, TEXT("bindHotbar"));
    }
}

void UInventoryRequestComponent::serverSelectHotbar_Implementation(int32 slotIndex){
    const FGameplayTag blocked = gate(false, false);   // selectHotbarSlot checks carry itself and reports HandsFull
    UHotbarComponent* hotbar = UHotbarComponent::findFor(getPlayerState());
    if(blocked.IsValid() || hotbar == nullptr){
        refuse(blocked.IsValid() ? blocked : T_Inventory_FailReason_NotFound, TEXT("selectHotbar"));
        return;
    }
    const FS_InventoryResult selected = hotbar->selectHotbarSlot(slotIndex);
    if(!selected.succeeded()){
        refuse(selected.failReason, TEXT("selectHotbar"));
    }
}

void UInventoryRequestComponent::serverUseConsumable_Implementation(FName itemId, FGuid instanceId){
    const FGameplayTag blocked = gate(false, false);   // useConsumable checks state and carry itself
    if(blocked.IsValid()){
        refuse(blocked, TEXT("useConsumable"));
        return;
    }
    const FS_InventoryResult used = UBFL_InventoryConsume::useConsumable(getPlayerState(), itemId, instanceId);
    if(!used.succeeded()){
        refuse(used.failReason, TEXT("useConsumable"));
    }
}

void UInventoryRequestComponent::serverDropItem_Implementation(FName itemId, FGuid instanceId, int32 quantity){
    const FGameplayTag blocked = gate(true, false);
    if(blocked.IsValid() || !isValidQuantity(quantity)){
        refuse(blocked.IsValid() ? blocked : T_Inventory_FailReason_InvalidQuantity, TEXT("dropItem"));
        return;
    }
    const FS_InventoryResult dropped = UBFL_Inventory::dropFromInventory(getPlayerState(), itemId, instanceId, quantity);
    if(!dropped.succeeded()){
        refuse(dropped.failReason, TEXT("dropItem"));
    }
}

void UInventoryRequestComponent::serverPickupLoose_Implementation(ALooseItemActor* loose, int32 quantity){
    const FGameplayTag blocked = gate(false, false);   // pickups that need free hands are checked by tryPickupLoose (HandsFull)
    APlayerState* player = getPlayerState();
    const UInventoryServices* services = UInventoryServices::get(this);
    if(blocked.IsValid() || loose == nullptr || player == nullptr || services == nullptr){
        refuse(blocked.IsValid() ? blocked : T_Inventory_FailReason_NotFound, TEXT("pickupLoose"));
        return;
    }
    const APawn* pawn = player->GetPawn();
    const bool withinDistance = pawn != nullptr && FVector::Dist(pawn->GetActorLocation(), loose->GetActorLocation()) <= UItemInventorySettings::get()->containerRange;
    if(!withinDistance || !services->isInInteractRange(player, loose)){
        refuse(T_Inventory_FailReason_OutOfRange, TEXT("pickupLoose"));
        return;
    }
    const FS_InventoryResult picked = UBFL_Inventory::tryPickupLoose(player, loose, quantity);
    if(!picked.succeeded()){
        refuse(picked.failReason, TEXT("pickupLoose"));   // BagFull / HandsFull: the existing one-line refusal feedback (Doc B17)
    }
}

void UInventoryRequestComponent::clientRefused_Implementation(FGameplayTag reason){
    // The UI maps the reason tag to text through DT_ReasonText (Doc B17 3.14, Doc R10 3.9). Nothing else happens.
    INVENTORY_LOG(Log, this, "Request refused: %s", *reason.ToString());
}

void UInventoryRequestComponent::clientGrantDropped_Implementation(FName itemId, int32 quantity){
    FS_GrantDroppedMessage message;
    message.player = getPlayerState();
    message.itemId = itemId;
    message.quantity = quantity;
    UGameplayMessageSubsystem::Get(this).BroadcastMessage(T_Inventory_GrantDropped, message);
}
