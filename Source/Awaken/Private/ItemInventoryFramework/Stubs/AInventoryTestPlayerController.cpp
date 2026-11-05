#include "ItemInventoryFramework/Stubs/AInventoryTestPlayerController.h"
#include "ItemInventoryFramework/UInventoryRequestComponent.h"

AInventoryTestPlayerController::AInventoryTestPlayerController(){
    requestComponent = CreateDefaultSubobject<UInventoryRequestComponent>(TEXT("InventoryRequests"));
}
