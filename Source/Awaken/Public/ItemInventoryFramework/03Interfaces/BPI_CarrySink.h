#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_CarrySink.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_CarrySink : public UInterface{
    GENERATED_BODY()
};

class IBPI_CarrySink{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool hasFreeCarrySlot(APlayerState* player, int32 carryCost);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool beginCarry(APlayerState* player, FName itemId);
};
