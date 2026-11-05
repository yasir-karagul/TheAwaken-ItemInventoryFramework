#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UInventoryServices::Initialize(FSubsystemCollectionBase& Collection){
    Super::Initialize(Collection);
    INVENTORY_LOG(Log, this, "Item & Inventory framework started");
}

UInventoryServices* UInventoryServices::get(const UObject* worldContext){
    UGameInstance* gameInstance = UGameplayStatics::GetGameInstance(worldContext);
    if(gameInstance == nullptr){
        return nullptr;
    }
    return gameInstance->GetSubsystem<UInventoryServices>();
}