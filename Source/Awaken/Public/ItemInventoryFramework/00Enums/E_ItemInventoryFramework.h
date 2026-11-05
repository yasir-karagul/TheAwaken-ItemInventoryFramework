#pragma once

#include "CoreMinimal.h"
#include "E_ItemInventoryFramework.generated.h"

// R01 §3.2: stackable = ItemID + quantity; instanced = own identity and state.
UENUM(BlueprintType)
enum class E_ItemKind : uint8{
    Stackable,
    Instanced
};

// R01 §3.4: the three inventory types.
UENUM(BlueprintType)
enum class E_InventoryType : uint8{
    Personal,
    Container,
    PortableContainer
};

// R01 §3.1a: five bag pages, no "All" page.
UENUM(BlueprintType)
enum class E_InventoryPage : uint8{
    Weapons,
    Tools,
    Consumables,
    Materials,
    Others
};

// R01 §3.2a / §3.1: what one consume-effect entry does.
UENUM(BlueprintType)
enum class E_ConsumeEffectKind : uint8{
    MeterChange,
    IllnessChance,
    HealthLoss
};
