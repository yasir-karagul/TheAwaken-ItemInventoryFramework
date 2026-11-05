#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UInventoryLayoutSubsystem.generated.h"

class UDA_InventoryCategoryMap;
class UDA_InventoryPageLayout;
class APlayerState;

UCLASS()
class AWAKEN_API UInventoryLayoutSubsystem : public UGameInstanceSubsystem{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static UInventoryLayoutSubsystem* get(const UObject* worldContext);

    // ---- queries (UI-facing; work on server and on the owning client) ----
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") E_InventoryPage getPageForItem(FName itemId) const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") FName getSlotForItem(FName itemId) const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") TArray<FS_InventorySlotDef> getSlotsForPage(E_InventoryPage page) const;
    // Every slot of the page with what the player holds in it. An empty slot is still returned (always drawn).
    UFUNCTION(BlueprintCallable, Category = "Awaken|Inventory") TArray<FS_SlotView> getSlotView(APlayerState* player, E_InventoryPage page) const;
    // Hover card data: each copy of an instanced item and which one is next to be used. Display only.
    UFUNCTION(BlueprintCallable, Category = "Awaken|Inventory") TArray<FS_InstanceCopyView> getInstanceCopies(APlayerState* player, FName itemId) const;

    // Cross-asset editor checks (S10.4, S10.5). True when there are no errors.
    bool validateLayout(TArray<FString>& errors) const;

private:
    void rebuild();

    UPROPERTY() TObjectPtr<UDA_InventoryCategoryMap> categoryMap;
    UPROPERTY() TMap<E_InventoryPage, TObjectPtr<UDA_InventoryPageLayout>> layouts;

    TMap<FName, E_InventoryPage> itemToPage;
    TMap<FName, FName> itemToSlot;
    // page -> slotId -> the one item, or the two items of a declared variant pair
    TMap<E_InventoryPage, TMap<FName, TArray<FName>>> slotToItems;
};
