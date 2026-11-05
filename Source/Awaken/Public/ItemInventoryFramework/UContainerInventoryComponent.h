#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UContainerInventoryComponent.generated.h"

class UInventoryData;

UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UContainerInventoryComponent : public UActorComponent{
    GENERATED_BODY()

public:
    virtual void OnComponentCreated() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Stable id (Doc02 §19.3). Designer-placed: editor assigns it. Player-built: assigned at creation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container") FGuid containerId;
    // PLACEHOLDER: per-type limit (R01 §3.4: containers use the bag's model; limits not decided).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container") int32 perTypeLimit = 20;
    // Empty = anything allowed (R01 §3.4 AllowedTags).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container") FGameplayTagContainer allowedTags;

    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory")
    UInventoryData* getInventory() const;
};
