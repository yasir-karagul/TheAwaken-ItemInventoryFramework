// TEMPORARY STUB — owner: Doc R13 Save (ISaveParticipant). Delete when R13's interface exists; rename the includes.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_SaveParticipantStub.generated.h"

UENUM(BlueprintType)
enum class E_SaveScope : uint8{
    World,
    Character
};

UINTERFACE(BlueprintType)
class UBPI_SaveParticipant : public UInterface{
    GENERATED_BODY()
};

class IBPI_SaveParticipant{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) FName getSaveKey();
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) E_SaveScope getSaveScope();
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) int32 getSnapshotVersion();
    // Query: never mutates. Opaque bytes; R13 decides how they are stored.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) TArray<uint8> getSnapshotForSave();
    // Command.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) bool restoreFromSnapshot(const TArray<uint8>& snapshot);
};
