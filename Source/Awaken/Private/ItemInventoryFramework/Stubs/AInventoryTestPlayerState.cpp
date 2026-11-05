// TEMPORARY STUB — owner: Doc B19.
#include "ItemInventoryFramework/Stubs/AInventoryTestPlayerState.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"

AInventoryTestPlayerState::AInventoryTestPlayerState(){
    inventoryComponent = CreateDefaultSubobject<UPlayerInventoryComponent>(TEXT("PlayerInventory"));
}

AInventoryTestGameMode::AInventoryTestGameMode(){
    PlayerStateClass = AInventoryTestPlayerState::StaticClass();
}
