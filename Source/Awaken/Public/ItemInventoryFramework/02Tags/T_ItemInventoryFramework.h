#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ---- Item categories and flags (R01 §3.1, §3.1a) ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Weapon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Tool);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Armor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Clothing);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Medicine);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Food);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Water);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Material);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Category_Container);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Tool_Harvest);       // Doc B08 3.2 (PROPOSED name)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Flag_Permanent);     // D10: the Axe and the map
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Water_Clean);        // R01 §3.4: two separate ItemIDs, told apart by item tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Item_Water_Unclean);

// ---- Refusal reasons (Doc02 §6). The first three are named by R01; the rest are PROPOSED ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_ContainerNotEmpty);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_Permanent);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_CannotOpenHere);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NotServer);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NotFound);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_UnknownItem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_InvalidQuantity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_InsufficientQuantity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_LimitReached);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_ReservedQuantity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_WrongInventoryType);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_WrongItemKind);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_ItemNotAllowed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_MixedWater);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_DeathBagLocked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NestedContainer);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_HeavyItem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NoBag);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_AlreadyHasBag);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_Held);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NotConsumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_InvalidSlot);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_NotOwner);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_BagActorFailed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_GearReleaseFailed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_RateLimited);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_FailReason_OutOfRange);

// ---- Messages (R01 §6, Doc02 §11) ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_Changed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Inventory_GrantDropped);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Hotbar_Changed);

// ---- Tags OWNED BY OTHER FRAMEWORKS, referenced here (see Stub register) ----
// TEMPORARY duplicates: native tag registration tolerates the same name defined twice. Remove the
// definition here when the owner's header exists, and include the owner's header instead.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Interaction_FailReason_BagFull);          // Doc B18 (approved, Index A94)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Interaction_FailReason_HandsFull);        // Doc B18
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Interaction_FailReason_NotAllowedInState);// Doc B18
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Survival_Meter_Illness);                  // Doc B03
UE_DECLARE_GAMEPLAY_TAG_EXTERN(T_Cosmetic_Event_Interaction_ItemDropped);  // Doc B21 A.4