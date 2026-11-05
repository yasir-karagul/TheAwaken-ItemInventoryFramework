#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UItemCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection){
    Super::Initialize(Collection);
    loadAllDefinitions();
}

UItemCatalogSubsystem* UItemCatalogSubsystem::get(const UObject* worldContext){
    UGameInstance* gameInstance = UGameplayStatics::GetGameInstance(worldContext);
    if(gameInstance == nullptr){
        return nullptr;
    }
    return gameInstance->GetSubsystem<UItemCatalogSubsystem>();
}

void UItemCatalogSubsystem::loadAllDefinitions(){
    definitions.Reset();
    if(!UAssetManager::IsInitialized()){
        INVENTORY_LOG(Warning, this, "Asset Manager not initialized; the item catalog is empty");
        return;
    }
    UAssetManager& manager = UAssetManager::Get();
    TArray<FPrimaryAssetId> assetIds;
    manager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("Item")), assetIds);
    for(const FPrimaryAssetId& assetId : assetIds){
        const FSoftObjectPath path = manager.GetPrimaryAssetPath(assetId);
        UDA_ItemDefinition* definition = Cast<UDA_ItemDefinition>(path.TryLoad());
        if(definition == nullptr){
            INVENTORY_LOG(Error, this, "Could not load item asset %s", *assetId.ToString());
            continue;
        }
        if(definitions.Contains(definition->itemId)){
            INVENTORY_LOG(Error, this, "Duplicate itemId %s", *definition->itemId.ToString());
            continue;
        }
        definitions.Add(definition->itemId, definition);
    }
    rebuildDerivedTables();
    INVENTORY_LOG(Log, this, "Item catalog loaded: %d definitions", definitions.Num());
}

void UItemCatalogSubsystem::rebuildDerivedTables(){
    spoiledToBase.Reset();
    for(const TPair<FName, TObjectPtr<UDA_ItemDefinition>>& pair : definitions){
        const UDA_ItemDefinition* definition = pair.Value;
        if(definition != nullptr && definition->canSpoil && !definition->spoiledItemId.IsNone()){
            spoiledToBase.Add(definition->spoiledItemId, definition->itemId);
        }
    }
}

const UDA_ItemDefinition* UItemCatalogSubsystem::getDefinition(FName itemId) const{
    const TObjectPtr<UDA_ItemDefinition>* found = definitions.Find(itemId);
    if(found == nullptr){
        INVENTORY_LOG(Warning, this, "Unknown itemId %s", *itemId.ToString());
        return nullptr;
    }
    return found->Get();
}

const TMap<FName, TObjectPtr<UDA_ItemDefinition>>& UItemCatalogSubsystem::getAllDefinitions() const{
    return definitions;
}

TArray<FS_ConsumeEffect> UItemCatalogSubsystem::getEffectiveConsumeEffects(FName itemId) const{
    TArray<FS_ConsumeEffect> result;
    const UDA_ItemDefinition* definition = getDefinition(itemId);
    if(definition == nullptr){
        return result;
    }

    FName baseId = definition->effectBaseItemId;
    float effectiveness = definition->uncleanWaterEffectiveness;
    if(baseId.IsNone()){
        const FName* spoiledBase = spoiledToBase.Find(itemId);
        if(spoiledBase != nullptr){
            baseId = *spoiledBase;
            const UDA_ItemDefinition* baseDefinition = getDefinition(baseId);
            effectiveness = baseDefinition != nullptr ? baseDefinition->spoiledEffectiveness : 1.0f;
        }
    }

    if(!baseId.IsNone()){
        const UDA_ItemDefinition* baseDefinition = getDefinition(baseId);
        if(baseDefinition != nullptr){
            for(const FS_ConsumeEffect& baseEffect : baseDefinition->consumeEffects){
                if(baseEffect.kind == E_ConsumeEffectKind::MeterChange){
                    FS_ConsumeEffect scaled = baseEffect;
                    scaled.amount = baseEffect.amount * effectiveness;
                    result.Add(scaled);
                }
            }
        }
    }
    result.Append(definition->consumeEffects);
    return result;
}

bool UItemCatalogSubsystem::validateAll(TArray<FString>& errors) const{
    const UItemInventorySettings* settings = UItemInventorySettings::get();
    int32 unbreakableCount = 0;

    for(const TPair<FName, TObjectPtr<UDA_ItemDefinition>>& pair : definitions){
        const UDA_ItemDefinition* definition = pair.Value;
        const FString id = pair.Key.ToString();
        if(definition == nullptr){
            continue;
        }
        if(definition->isUnbreakable()){
            unbreakableCount++;
        }
        if(definition->canSpoil){
            const UDA_ItemDefinition* spoiled = definitions.Contains(definition->spoiledItemId) ? definitions[definition->spoiledItemId].Get() : nullptr;
            if(spoiled == nullptr){
                errors.Add(FString::Printf(TEXT("%s: spoiledItemId %s does not exist"), *id, *definition->spoiledItemId.ToString()));
            }else if(spoiled->canSpoil){
                errors.Add(FString::Printf(TEXT("%s: the spoiled variant %s must not itself spoil"), *id, *spoiled->itemId.ToString()));
            }
        }
        if(!definition->effectBaseItemId.IsNone() && !definitions.Contains(definition->effectBaseItemId)){
            errors.Add(FString::Printf(TEXT("%s: effectBaseItemId %s does not exist"), *id, *definition->effectBaseItemId.ToString()));
        }
        if(!definition->slotSharedWith.IsNone()){
            const TObjectPtr<UDA_ItemDefinition>* other = definitions.Find(definition->slotSharedWith);
            if(other == nullptr || other->Get() == nullptr){
                errors.Add(FString::Printf(TEXT("%s: slotSharedWith %s does not exist"), *id, *definition->slotSharedWith.ToString()));
            }else if((*other)->slotSharedWith != definition->itemId){
                errors.Add(FString::Printf(TEXT("%s: slotSharedWith %s does not link back"), *id, *definition->slotSharedWith.ToString()));
            }
        }
    }

    if(unbreakableCount > 1){
        errors.Add(FString::Printf(TEXT("%d items use the unbreakable sentinel; exactly one (the starter Axe) is allowed"), unbreakableCount));
    }
    if(!definitions.Contains(settings->unarmedItemId)){
        errors.Add(FString::Printf(TEXT("Unarmed item %s is missing"), *settings->unarmedItemId.ToString()));
    }
    for(const FName& kitId : settings->startingKitItemIds){
        if(!definitions.Contains(kitId)){
            errors.Add(FString::Printf(TEXT("Starting kit item %s is missing"), *kitId.ToString()));
        }
    }
    return errors.Num() == 0;
}

#if !UE_BUILD_SHIPPING
void UItemCatalogSubsystem::registerTransientDefinition(UDA_ItemDefinition* definition){
    if(definition != nullptr){
        definitions.Add(definition->itemId, definition);
        rebuildDerivedTables();
    }
}
#endif
