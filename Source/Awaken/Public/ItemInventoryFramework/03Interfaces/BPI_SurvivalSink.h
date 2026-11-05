#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "BPI_SurvivalSink.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_SurvivalSink : public UInterface{
    GENERATED_BODY()
};

class IBPI_SurvivalSink{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void modifyMeter(APlayerState* player, FGameplayTag meterTag, float delta);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void registerIllnessSource(APlayerState* player, FGameplayTag sourceTag, float durationHours);

    // Doc B13 3.3: a switched-off meter does not tick and applies nothing.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool isMeterActive(FGameplayTag meterTag);
};
