// TEMPORARY STUB — owner: Doc B19 Game Framework class table. The real PlayerState gets the same components.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "AInventoryTestPlayerState.generated.h"

class UPlayerInventoryComponent;

UCLASS()
class AInventoryTestPlayerState : public APlayerState{
    GENERATED_BODY()

public:
    AInventoryTestPlayerState();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UPlayerInventoryComponent> inventoryComponent;
    // Section 6 adds: hotbarComponent.
};

UCLASS()
class AInventoryTestGameMode : public AGameModeBase{
    GENERATED_BODY()

public:
    AInventoryTestGameMode();
};
