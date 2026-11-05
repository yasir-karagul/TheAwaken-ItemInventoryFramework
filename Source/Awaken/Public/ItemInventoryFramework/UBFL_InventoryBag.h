#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UBFL_InventoryBag.generated.h"

class APlayerState;
class UInventoryData;

UCLASS()
class AWAKEN_API UBFL_InventoryBag : public UBlueprintFunctionLibrary{
    GENERATED_BODY()

public:
    // Query (R01 §5).
    static bool hasBag(const APlayerState* player);

    // Command, server. Called by Doc B02 on a multiplayer death. Atomic (see the table above).
    static FS_BagDetachResult detachBag(APlayerState* player, const FVector& deathLocation);
    // Command, server. Recovery: one atomic step. Only the owning character may do it (Doc B02 4.3).
    static FS_InventoryResult reattachBag(APlayerState* player, const FGuid& bagId);
    // Command, server. A Bagless player who dies: the three hold-slot items are destroyed (R01 §3.8).
    static int32 discardBaglessItems(APlayerState* player);
    // Command, server. Once per character, guarded (R01 §3.8; Index A93 UD21).
    static FS_InventoryResult grantStartingKit(APlayerState* player);
    // Command, server. Fresh character / Start Again: old bags removed completely, new bag, new kit.
    static FS_InventoryResult startNewCharacter(APlayerState* player, const FGuid& newCharacterId);
    // Command, server. For Doc R13's DeleteCharacter.
    static void removeBagsOwnedBy(const UObject* worldContext, const FGuid& characterId);

private:
    static void storeOrDropBesideBag(APlayerState* player, UInventoryData* bag, const FS_InventoryEntry& item, const FVector& location);
};
