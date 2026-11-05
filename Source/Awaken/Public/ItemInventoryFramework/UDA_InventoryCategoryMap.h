#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "UDA_InventoryCategoryMap.generated.h"

// R01 §3.1a: the one data mapping from CategoryTag to page. A category not named goes to Others.
UCLASS(BlueprintType)
class AWAKEN_API UDA_InventoryCategoryMap : public UDataAsset{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FGameplayTag, E_InventoryPage> categoryToPage;

    E_InventoryPage getPageFor(const FGameplayTag& categoryTag) const{
        const E_InventoryPage* found = categoryToPage.Find(categoryTag);
        return found != nullptr ? *found : E_InventoryPage::Others;
    }
};
