#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "BPI_BagActorService.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_BagActorService : public UInterface{
    GENERATED_BODY()
};

class IBPI_BagActorService{
    GENERATED_BODY()

public:
    // Creates the bag record and the actor first (Doc B02 3.6). Returns an invalid guid on failure.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    FGuid createBagRecordAndActor(APlayerState* player, FVector location, FGuid characterId, FGuid inventoryId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool deleteBag(FGuid bagId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void finalizeBag(FGuid bagId, const FS_BagContents& contents);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool bagExists(FGuid bagId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    FS_BagContents getBagContents(FGuid bagId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    FGuid getBagOwner(FGuid bagId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    FVector getBagLocation(FGuid bagId);

    // Fresh character: removes every bag owned by the old CharacterID completely (R01 §3.8, R13 §3.8).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void removeBagsOwnedBy(FGuid characterId);
};
