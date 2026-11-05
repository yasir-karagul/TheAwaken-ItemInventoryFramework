#pragma once

#include "CoreMinimal.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "D_ItemInventoryFramework.generated.h"

class APlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD_InventoryChanged, const FGuid&, inventoryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD_HotbarChanged, APlayerState*, player, int32, slotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FD_GrantDropped, APlayerState*, player, FName, itemId, int32, quantity);
