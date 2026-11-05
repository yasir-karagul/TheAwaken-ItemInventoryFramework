#pragma once

#include "CoreMinimal.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"

#if WITH_DEV_AUTOMATION_TESTS

class UGameInstance;
class UWorld;
class UInventoryData;
class UItemCatalogSubsystem;
class UInventoryServices;
class UDA_ItemDefinition;
class AInventoryTestPlayerState;

// Builds a standalone game instance + world so subsystems exist. Best effort: if your engine build needs
// a different bootstrap (for example a PIE world), change only this file.
struct FInventoryTestFixture{
    FInventoryTestFixture();
    ~FInventoryTestFixture();

    UGameInstance* gameInstance = nullptr;
    UWorld* world = nullptr;

    UItemCatalogSubsystem* catalog() const;
    UInventoryServices* services() const;

    UDA_ItemDefinition* addStackable(FName itemId, int32 limit);
    UDA_ItemDefinition* addInstanced(FName itemId, int32 limit, float durability);
    AInventoryTestPlayerState* makePlayer();
    UDA_ItemDefinition* addSpoilable(FName itemId, FName spoiledItemId, float spoilHours);
    UInventoryData* makeInventory(E_InventoryType type = E_InventoryType::Personal);
};

#endif
