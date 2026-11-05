#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UDA_InventoryPageLayout.generated.h"

// One page of the bag screen. A slotId is unique inside the layout and is never renumbered or reused (R01 §3.1b).
UCLASS(BlueprintType)
class AWAKEN_API UDA_InventoryPageLayout : public UDataAsset{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) E_InventoryPage page = E_InventoryPage::Others;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FS_InventorySlotDef> slots;

    const FS_InventorySlotDef* findSlot(FName slotId) const{
        for(const FS_InventorySlotDef& slot : slots){
            if(slot.slotId == slotId){
                return &slot;
            }
        }
        return nullptr;
    }
};
