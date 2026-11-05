#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UDA_ItemDefinition::GetPrimaryAssetId() const{
    return FPrimaryAssetId(FPrimaryAssetType(TEXT("Item")), itemId);
}

FName UDA_ItemDefinition::getDisplayNameKey() const{
    if(!displayNameKeyOverride.IsNone()){
        return displayNameKeyOverride;
    }
    return FName(*FString::Printf(TEXT("Item.%s.Name"), *itemId.ToString()));
}

FName UDA_ItemDefinition::getDescriptionKey() const{
    if(!descriptionKeyOverride.IsNone()){
        return descriptionKeyOverride;
    }
    return FName(*FString::Printf(TEXT("Item.%s.Description"), *itemId.ToString()));
}

#if WITH_EDITOR
EDataValidationResult UDA_ItemDefinition::IsDataValid(FDataValidationContext& Context) const{
    EDataValidationResult result = Super::IsDataValid(Context);
    auto fail = [&Context, &result](const FString& message){
        Context.AddError(FText::FromString(message));
        result = EDataValidationResult::Invalid;
    };

    if(itemId.IsNone()){
        fail(TEXT("itemId is empty."));
    }
    if(!categoryTag.IsValid()){
        fail(TEXT("categoryTag is empty."));
    }
    if(!isHeavy() && perTypeLimit < 1){
        fail(TEXT("perTypeLimit must be at least 1 for an item that can enter the bag."));
    }
    if(canSpoil){
        if(spoilTime <= 0.0f){
            fail(TEXT("canSpoil is on but spoilTime is not above zero."));
        }
        if(spoiledItemId.IsNone()){
            fail(TEXT("canSpoil is on but spoiledItemId is empty."));
        }
        if(spoiledItemId == itemId){
            fail(TEXT("spoiledItemId points at the item itself."));
        }
    }
    if(isPortableContainer()){
        if(itemKind != E_ItemKind::Instanced){
            fail(TEXT("A Portable Container must be Instanced (R01 3.4)."));
        }
        if(containerPerTypeLimit < 1){
            fail(TEXT("A Portable Container needs containerPerTypeLimit of at least 1."));
        }
    }
    if(isWeapon() && weaponType == E_WeaponType::Firearm && magazineSize < 1){
        fail(TEXT("A Firearm needs magazineSize of at least 1."));
    }
    if(isWeapon() && durabilityMax == 0.0f){
        Context.AddWarning(FText::FromString(TEXT("Weapon has durabilityMax 0. Use the unbreakable sentinel only for the starter Axe.")));
    }
    for(const FS_ConsumeEffect& effect : consumeEffects){
        if(effect.kind == E_ConsumeEffectKind::IllnessChance && (effect.amount < 0.0f || effect.amount > 1.0f)){
            fail(TEXT("An IllnessChance effect must have amount between 0 and 1."));
        }
        if(effect.kind != E_ConsumeEffectKind::HealthLoss && !effect.tag.IsValid()){
            fail(TEXT("A MeterChange or IllnessChance effect needs a tag."));
        }
    }
    return result;
}
#endif
