#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "UItemInventorySettings.generated.h"

class UDA_InventoryCategoryMap;
class UDA_InventoryPageLayout;

// Project Settings -> Game -> Awaken Item Inventory. Every value is PLACEHOLDER tuning data (Doc00 2.7).
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Awaken Item Inventory"))
class AWAKEN_API UItemInventorySettings : public UDeveloperSettings{
    GENERATED_BODY()

public:
    UItemInventorySettings();

    static const UItemInventorySettings* get(){
        return GetDefault<UItemInventorySettings>();
    }

    // R01 §3.10: the always-available Unarmed entry. Never stored in an inventory.
    UPROPERTY(Config, EditAnywhere, Category = "Items") FName unarmedItemId = TEXT("Unarmed");
    // R01 §3.8: granted once inside the new bag (the Axe and the map).
    UPROPERTY(Config, EditAnywhere, Category = "Items") TArray<FName> startingKitItemIds;

    UPROPERTY(Config, EditAnywhere, Category = "Layout") TSoftObjectPtr<UDA_InventoryCategoryMap> categoryMap;
    UPROPERTY(Config, EditAnywhere, Category = "Layout") TMap<E_InventoryPage, TSoftObjectPtr<UDA_InventoryPageLayout>> pageLayouts;

    // Doc02 §19.7: default loose lifetime when an item's own value is 0.
    UPROPERTY(Config, EditAnywhere, Category = "Loose") float defaultLooseLifetimeSeconds = 300.0f;
    UPROPERTY(Config, EditAnywhere, Category = "Loose") float dropForwardOffset = 100.0f;

    // Section 12: range check until Doc B18's hook exists, and request throttling.
    UPROPERTY(Config, EditAnywhere, Category = "Network") float containerRange = 300.0f;
    UPROPERTY(Config, EditAnywhere, Category = "Network") float requestsPerSecond = 20.0f;
    UPROPERTY(Config, EditAnywhere, Category = "Network") int32 maxRequestQuantity = 9999;
};
