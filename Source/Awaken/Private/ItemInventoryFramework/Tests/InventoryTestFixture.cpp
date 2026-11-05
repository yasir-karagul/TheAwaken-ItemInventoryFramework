#include "ItemInventoryFramework/Tests/InventoryTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ItemInventoryFramework/Stubs/AInventoryTestPlayerState.h"
#include "ItemInventoryFramework/Stubs/InventoryStubs.h"
#include "ItemInventoryFramework/03Interfaces/BPI_GameClock.h"

FInventoryTestFixture::FInventoryTestFixture(){
    gameInstance = NewObject<UGameInstance>(GEngine);
    gameInstance->AddToRoot();
    gameInstance->InitializeStandalone();
    world = gameInstance->GetWorld();
}

FInventoryTestFixture::~FInventoryTestFixture(){
    if(gameInstance != nullptr){
        gameInstance->Shutdown();
        gameInstance->RemoveFromRoot();
    }
}

UItemCatalogSubsystem* FInventoryTestFixture::catalog() const{
    return gameInstance->GetSubsystem<UItemCatalogSubsystem>();
}

UInventoryServices* FInventoryTestFixture::services() const{
    return gameInstance->GetSubsystem<UInventoryServices>();
}

UDA_ItemDefinition* FInventoryTestFixture::addStackable(FName itemId, int32 limit){
    UDA_ItemDefinition* definition = NewObject<UDA_ItemDefinition>(gameInstance);
    definition->itemId = itemId;
    definition->itemKind = E_ItemKind::Stackable;
    definition->categoryTag = T_Item_Category_Material;
    definition->perTypeLimit = limit;
    catalog()->registerTransientDefinition(definition);
    return definition;
}

UDA_ItemDefinition* FInventoryTestFixture::addInstanced(FName itemId, int32 limit, float durability){
    UDA_ItemDefinition* definition = NewObject<UDA_ItemDefinition>(gameInstance);
    definition->itemId = itemId;
    definition->itemKind = E_ItemKind::Instanced;
    definition->categoryTag = T_Item_Category_Weapon;
    definition->perTypeLimit = limit;
    definition->durabilityMax = durability;
    catalog()->registerTransientDefinition(definition);
    return definition;
}

UInventoryData* FInventoryTestFixture::makeInventory(E_InventoryType type){
    UInventoryData* inventory = NewObject<UInventoryData>(world);
    inventory->initialize(FGuid::NewGuid(), type);
    return inventory;
}


AInventoryTestPlayerState* FInventoryTestFixture::makePlayer(){
    // Components only BeginPlay in a world that has begun play. If your bootstrap leaves the world idle, start it here.
    if(!world->HasBegunPlay()){
        world->InitializeActorsForPlay(FURL());
        world->BeginPlay();
    }
    return world->SpawnActor<AInventoryTestPlayerState>();
}

UDA_ItemDefinition* FInventoryTestFixture::addSpoilable(FName itemId, FName spoiledItemId, float spoilHours){
    UDA_ItemDefinition* fresh = addStackable(itemId, 10);
    fresh->canSpoil = true;
    fresh->spoilTime = spoilHours;
    fresh->spoiledItemId = spoiledItemId;
    fresh->categoryTag = T_Item_Category_Food;
    UDA_ItemDefinition* spoiled = addStackable(spoiledItemId, 10);
    spoiled->categoryTag = T_Item_Category_Food;
    return fresh;
}

#endif
