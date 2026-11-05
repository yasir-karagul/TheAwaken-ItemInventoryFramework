#include "ItemInventoryFramework/UHeldItemComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"

FS_HeldItemView UBFL_Inventory::getHeldItem(const APlayerState* player){
    const UHeldItemComponent* held = UHeldItemComponent::findFor(player != nullptr ? player->GetPawn() : nullptr);
    if(held == nullptr){
        FS_HeldItemView unarmed;
        unarmed.itemId = UItemInventorySettings::get()->unarmedItemId;
        return unarmed;
    }
    return held->getHeldView();
}

const UDA_ItemDefinition* UBFL_Inventory::getHeldDefinition(const APlayerState* player){
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(player);
    if(catalog == nullptr){
        return nullptr;
    }
    const UDA_ItemDefinition* definition = catalog->getDefinition(getHeldItem(player).itemId);
    return definition != nullptr ? definition : catalog->getDefinition(UItemInventorySettings::get()->unarmedItemId);
}

bool UBFL_Inventory::isInstanceHeld(const APlayerState* player, const FGuid& instanceId){
    const UHeldItemComponent* held = UHeldItemComponent::findFor(player != nullptr ? player->GetPawn() : nullptr);
    return held != nullptr && held->isInstanceHeld(instanceId);
}
