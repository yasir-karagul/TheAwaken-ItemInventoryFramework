#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"
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
    UInventoryRegistrySubsystem* registry = UInventoryRegistrySubsystem::get(this);
    if(registry != nullptr){
        registry->registerInventory(personalInventory);
    }
    AddReplicatedSubObject(personalInventory, COND_OwnerOnly);
    INVENTORY_LOG(Log, this, "Personal inventory %s created for %s", *personalInventory->getInventoryId().ToString(), *GetOwner()->GetName());
    bindPersonalInventory();
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


#include "ItemInventoryFramework/UBFL_InventorySave.h"
#include "ItemInventoryFramework/UHotbarComponent.h"

FName UPlayerInventoryComponent::getSaveKey_Implementation(){
    return TEXT("Inventory.Character");
}

E_SaveScope UPlayerInventoryComponent::getSaveScope_Implementation(){
    return E_SaveScope::Character;
}

int32 UPlayerInventoryComponent::getSnapshotVersion_Implementation(){
    return UBFL_InventorySave::currentSnapshotVersion;
}

TArray<uint8> UPlayerInventoryComponent::getSnapshotForSave_Implementation(){
    // R13 excludes Bagless players from capture (a Character file never says Bagless), so no bag = no snapshot.
    if(!hasBag()){
        return TArray<uint8>();
    }
    FS_CharacterInventorySave record;
    record.version = UBFL_InventorySave::currentSnapshotVersion;
    record.personal = UBFL_InventorySave::captureBundle(personalInventory, true);   // Character file: time LEFT (D3)
    const UHotbarComponent* hotbar = UHotbarComponent::findFor(Cast<APlayerState>(GetOwner()));
    if(hotbar != nullptr){
        record.hotbar.bindings = hotbar->getBindingsSnapshot();   // bindings only; reservations/held/hold slots/last-used are NOT saved
    }
    return UBFL_InventorySave::toBytes(FS_CharacterInventorySave::StaticStruct(), &record);
}

bool UPlayerInventoryComponent::restoreFromSnapshot_Implementation(const TArray<uint8>& snapshot){
    FS_CharacterInventorySave record;
    if(!UBFL_InventorySave::fromBytes(FS_CharacterInventorySave::StaticStruct(), &record, snapshot) || !UBFL_InventorySave::migrateCharacterSave(record)){
        return false;   // refused; R13 shows the reason and a load failure never overwrites a file
    }
    if(!hasBag()){
        createPersonalInventory();   // B06 decides when a player stays Bagless (P4 exception) and then does not call this
    }
    releaseAllReservations();        // after a reload there are none
    UBFL_InventorySave::applyBundle(personalInventory, record.personal, true);
    UHotbarComponent* hotbar = UHotbarComponent::findFor(Cast<APlayerState>(GetOwner()));
    if(hotbar != nullptr){
        hotbar->restoreBindings(record.hotbar.bindings);
    }
    return true;
}
