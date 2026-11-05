#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_GameClock.generated.h"

UINTERFACE(BlueprintType)
class UBPI_GameClock : public UInterface{
    GENERATED_BODY()
};

class IBPI_GameClock{
    GENERATED_BODY()

public:
    // Game seconds since the world was created, 64-bit whole number (Doc02 §19.4). One game hour = 3600 game seconds.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    int64 getGameTimeSeconds();
};