#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_VitalsSink.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_VitalsSink : public UInterface{
    GENERATED_BODY()
};

class IBPI_VitalsSink{
    GENERATED_BODY()

public:
    // Must go through the Doc R03 3.7 pipeline with DamageType.Illness (bypasses armor, Block, Dodge).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void applyIllnessDamage(APlayerState* player, float amount);

    // Doc R08 ModifyHP with a positive amount. Medicine is not limited by the regeneration cap.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void restoreHealth(APlayerState* player, float amount);
};
