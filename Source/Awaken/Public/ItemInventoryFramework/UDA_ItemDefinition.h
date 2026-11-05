#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemInventoryFramework/00Enums/E_ItemInventoryFramework.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"
#include "ItemInventoryFramework/03Interfaces/E_WeaponTypeStub.h"
#include "ItemInventoryFramework/ItemInventoryConstants.h"
#include "UDA_ItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;
class UDA_StackLayout;

UCLASS(BlueprintType)
class AWAKEN_API UDA_ItemDefinition : public UPrimaryDataAsset{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    // ---------- Identity and presentation (R01 §3.1) ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FName itemId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") E_ItemKind itemKind = E_ItemKind::Stackable;
    // Optional overrides; normally the key is derived from itemId (Doc R10 §3.3).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FName displayNameKeyOverride;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FName descriptionKeyOverride;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") TSoftObjectPtr<UTexture2D> icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") TSoftObjectPtr<UStaticMesh> worldMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Item.Category")) FGameplayTag categoryTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FGameplayTagContainer itemTags;

    // ---------- Limits and carry (R01 §3.1) ----------
    // The most of this item the Personal Inventory can hold. Unused when carryCapacityCost > 0.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "0")) int32 perTypeLimit = 1;
    // > 0 means carry-only: the item never enters the bag (Doc B07 3.1).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "0")) int32 carryCapacityCost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits") FGameplayTag weightClassTag;
    // D11: seconds a loose copy lives; 0 = project default (Doc02 §19.7).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "0")) float looseLifetimeSeconds = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Processing") bool requiresProcessing = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Processing") FPrimaryAssetId linkedProcessingRecipe;

    // ---------- Weapon block (R01 §3.1). Combat reads these; it does not own them ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") E_WeaponType weaponType = E_WeaponType::Melee;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float attack = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float attackSpeed = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float damage = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float armorPenetrationPercent = 0.0f;
    // unbreakableDurability (-1) = never wears out. 0 = no durability (Unarmed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float durabilityMax = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float durabilityLossPerUse = 1.0f;

    // ---------- Firearm fields (R01 §3.1, Index A116). Names PROPOSED, every value data ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") int32 magazineSize = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") float bulletSpeed = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") float bulletDrop = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") float longestRange = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") float recoilPitch = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") float recoilYaw = 0.0f;

    // ---------- Consume block (R01 §3.1, §3.2a, §3.9) ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consume") TArray<FS_ConsumeEffect> consumeEffects;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consume") float healthRestoreAmount = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consume") float illnessReductionAmount = 0.0f;

    // ---------- Spoilage (R01 §3.2a). canSpoil is the spec's bCanSpoil ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage") bool canSpoil = false;
    // In-game hours.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage", meta = (EditCondition = "canSpoil")) float spoilTime = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage", meta = (EditCondition = "canSpoil")) FName spoiledItemId;
    // Share of the fresh item's meter effects that the spoiled variant gives (designer: 0.5).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage", meta = (EditCondition = "canSpoil", ClampMin = "0", ClampMax = "1")) float spoiledEffectiveness = 0.5f;
    // Set on the UNCLEAN item: share of the clean item's thirst effect (designer: 0.5).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage", meta = (ClampMin = "0", ClampMax = "1")) float uncleanWaterEffectiveness = 1.0f;
    // D8 (PROPOSED): the item whose MeterChange effects this variant scales. Empty for ordinary items.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoilage") FName effectBaseItemId;

    // ---------- Container block (R01 §3.1; Portable Container items only) ----------
    // The spec's CapacityRules, PROPOSED shape: one limit per item type, same model as the bag.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container") int32 containerPerTypeLimit = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container") FGameplayTagContainer allowedTags;
    // Equipment block: Doc R05 fills this in later. Nothing here on purpose.

    // ---------- Bag screen slot data (R01 §3.1, §3.1a) ----------
    // The page is derived from categoryTag, so only the slot id is stored (R01 §3.1a: "the page and the category can never disagree").
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot") FName inventorySlot;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot") TSoftObjectPtr<UStaticMesh> inventoryMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot") FName slotSharedWith;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot") TSoftObjectPtr<UDA_StackLayout> stackLayout;

    // ---------- Helpers (Query) ----------
    bool isHeavy() const{
        return carryCapacityCost > 0;
    }

    bool isPermanent() const{
        return itemTags.HasTag(T_Item_Flag_Permanent);
    }

    bool isUnbreakable() const{
        return durabilityMax == ItemInventoryConstants::unbreakableDurability;
    }

    bool isPortableContainer() const{
        return categoryTag == T_Item_Category_Container;
    }

    bool isWeapon() const{
        return categoryTag == T_Item_Category_Weapon;
    }

    bool isConsumable() const{
        return consumeEffects.Num() > 0 || healthRestoreAmount > 0.0f || illnessReductionAmount > 0.0f;
    }

    FName getDisplayNameKey() const;
    FName getDescriptionKey() const;
};
