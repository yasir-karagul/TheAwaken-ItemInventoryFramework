#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UItemCatalogSubsystem.generated.h"

class UDA_ItemDefinition;

UCLASS()
class AWAKEN_API UItemCatalogSubsystem : public UGameInstanceSubsystem{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static UItemCatalogSubsystem* get(const UObject* worldContext);

    // Query. Returns null (and logs once per id) for an unknown id.
    const UDA_ItemDefinition* getDefinition(FName itemId) const;
    const TMap<FName, TObjectPtr<UDA_ItemDefinition>>& getAllDefinitions() const;

    // Query. The meter/illness/HP effects that actually apply when this item is consumed,
    // with the spoiled / unclean effectiveness scaling already applied (D8, R01 §3.2a).
    TArray<FS_ConsumeEffect> getEffectiveConsumeEffects(FName itemId) const;

    // Query. Cross-asset checks (R01 §3.1, §3.1a). Returns true when there are no errors.
    bool validateAll(TArray<FString>& errors) const;

#if !UE_BUILD_SHIPPING
    // Test hook: add a definition that is not an asset on disk.
    void registerTransientDefinition(UDA_ItemDefinition* definition);
#endif

private:
    void loadAllDefinitions();
    void rebuildDerivedTables();

    UPROPERTY() TMap<FName, TObjectPtr<UDA_ItemDefinition>> definitions;
    // spoiled item id -> fresh item id (derived from spoiledItemId)
    TMap<FName, FName> spoiledToBase;
};
