#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "BPI_EquipmentGear.generated.h"

class APlayerState;

UINTERFACE(BlueprintType)
class UBPI_EquipmentGear : public UInterface{
    GENERATED_BODY()
};

class IBPI_EquipmentGear{
    GENERATED_BODY()

public:
    // Doc R05 3.3 ReleaseEquippedGear: returns every worn armor instance (full entries) and the slot each held.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    FS_GearReleaseResult releaseEquippedGear(APlayerState* player);

    // Re-equips the listed instances in the slots they occupied (R01 §3.8 step 4).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool reEquipGear(APlayerState* player, const TArray<FS_GearSlotRecord>& gear);
};
