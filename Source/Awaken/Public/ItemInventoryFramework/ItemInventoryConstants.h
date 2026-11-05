#pragma once

#include "CoreMinimal.h"

namespace ItemInventoryConstants{
    // R01 §3.1: one named sentinel, not "a very large number". Only the starter Axe uses it (R01 §3.10).
    constexpr float unbreakableDurability = -1.0f;
    constexpr int32 hotbarSlotCount = 10;          // R01 §3.5: slots 0 to 9 (D12)
    constexpr int32 baglessHoldSlotCount = 3;      // R01 §3.8: keys 1, 2, 3
    constexpr int64 gameSecondsPerGameHour = 3600; // Doc02 §19.4 / Doc R12 3.1
}
