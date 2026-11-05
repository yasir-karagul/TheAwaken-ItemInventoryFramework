// TEMPORARY STUB — owner: Doc B19.
#include "ItemInventoryFramework/Stubs/AInventoryTestPlayerState.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"

AInventoryTestPlayerState::AInventoryTestPlayerState(){
    inventoryComponent = CreateDefaultSubobject<UPlayerInventoryComponent>(TEXT("PlayerInventory"));
    hotbarComponent = CreateDefaultSubobject<UHotbarComponent>(TEXT("Hotbar"));
}

AInventoryTestGameMode::AInventoryTestGameMode(){
    PlayerStateClass = AInventoryTestPlayerState::StaticClass();
}
