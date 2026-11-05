#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "BPI_LooseItemSpawner.generated.h"

UINTERFACE(BlueprintType)
class UBPI_LooseItemSpawner : public UInterface{
    GENERATED_BODY()
};

class IBPI_LooseItemSpawner{
    GENERATED_BODY()

public:
    // Spawns one loose body that carries the whole entry (stack or instance). Server only. Never saved.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    AActor* spawnLooseItem(UObject* worldContext, const FS_InventoryEntry& entry, FVector location);
};
