#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UDA_StackLayout.generated.h"

// Where unit 1, 2, 3 ... of a pile sit inside the slot pocket and how each is rotated (R01 §3.1a).
// The visible capacity of a pile is units.Num(); there is no separate cap setting.
UCLASS(BlueprintType)
class AWAKEN_API UDA_StackLayout : public UDataAsset{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FS_StackUnitPlacement> units;
    // Half-extent of the slot pocket, used by the "a full pile fits its pocket" data check.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector pocketHalfExtent = FVector(10.0, 10.0, 10.0);
};
