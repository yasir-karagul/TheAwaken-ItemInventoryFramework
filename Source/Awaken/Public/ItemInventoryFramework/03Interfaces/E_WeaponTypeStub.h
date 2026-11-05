// TEMPORARY STUB — owner: Doc R03 Combat. Delete when R03's E_WeaponType exists (same name, same values).
#pragma once

#include "CoreMinimal.h"
#include "E_WeaponTypeStub.generated.h"

UENUM(BlueprintType)
enum class E_WeaponType : uint8{
    Melee,
    Bow,
    Crossbow,
    Firearm,
    Thrown
};