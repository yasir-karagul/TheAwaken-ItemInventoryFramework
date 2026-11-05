#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_PlayerStateQuery.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_PlayerStateQuery : public UInterface{
    GENERATED_BODY()
};

class IBPI_PlayerStateQuery{
    GENERATED_BODY()

public:
    // Downed, Restrained, Arriving, scene lock: the tag Player.State.Incapacitated (Doc B02, R04 3.3).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool isIncapacitated(APlayerState* player);

    // Any carry slot occupied (Doc B07 IsCarryActive).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool isCarryActive(APlayerState* player);

    // Range from the pawn's eye plus line of sight (Doc B18 3.5). Inclusive at the limit (Doc02 §19.9).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool isInInteractRange(APlayerState* player, AActor* target);
};
