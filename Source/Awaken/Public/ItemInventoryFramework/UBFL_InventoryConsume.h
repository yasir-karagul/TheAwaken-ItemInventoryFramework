#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UBFL_InventoryConsume.generated.h"

class UInventoryData;
class APlayerState;

UCLASS()
class AWAKEN_API UBFL_InventoryConsume : public UBlueprintFunctionLibrary{
    GENERATED_BODY()

public:
    // Command, server. Converts one stack if its spoil time has passed. Never per tick.
    static FS_SpoilResult evaluateSpoilage(UInventoryData* inventory, FName itemId);
    // Command, server. Every spoilable stack of the inventory. Returns the conversions that happened.
    static TArray<FS_SpoilResult> evaluateAllSpoilage(UInventoryData* inventory);
    // Command, server. Same, for a player's bag, and clears the reservations of converted stacks.
    static void evaluateSpoilageForPlayer(APlayerState* player);

    // Command, server. Eat, drink or take medicine (R01 §3.9). Explained refusal, nothing partial.
    UFUNCTION(BlueprintCallable, Category = "Awaken|Inventory")
    static FS_InventoryResult useConsumable(APlayerState* player, FName itemId, const FGuid& instanceId);
};
