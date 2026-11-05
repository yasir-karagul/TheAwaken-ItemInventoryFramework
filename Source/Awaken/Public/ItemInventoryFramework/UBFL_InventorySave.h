#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UBFL_InventorySave.generated.h"

class UInventoryData;

UCLASS()
class AWAKEN_API UBFL_InventorySave : public UBlueprintFunctionLibrary{
    GENERATED_BODY()

public:
    inline static constexpr int32 currentSnapshotVersion = 1;

    // Query + lazy spoilage: evaluates spoilage first (S8.4), then snapshots. useTimeLeft = Character file, false = World file (D3).
    static FS_InventorySaveBundle captureBundle(UInventoryData* inventory, bool useTimeLeft);
    // Command, server. Replaces the CONTENT of an existing inventory object; recreates nested Portable Container inventories.
    static void applyBundle(UInventoryData* target, const FS_InventorySaveBundle& bundle, bool useTimeLeft);

    // Bytes <-> USTRUCT, using the same proxy archive the engine's SaveGame uses.
    static TArray<uint8> toBytes(UScriptStruct* structType, const void* structData);
    static bool fromBytes(UScriptStruct* structType, void* structData, const TArray<uint8>& bytes);

    // S11.7: chained migration, one step per version. Returns false when a step is missing (the file is refused).
    static bool migrateCharacterSave(FS_CharacterInventorySave& record);
    static bool migrateWorldSave(FS_WorldInventorySave& record);

private:
    static FS_InventorySaveRecord makeRecord(UInventoryData* inventory, bool useTimeLeft);
};
