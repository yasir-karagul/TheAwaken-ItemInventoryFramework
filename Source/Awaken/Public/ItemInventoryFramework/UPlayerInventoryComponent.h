#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UPlayerInventoryComponent.generated.h"

class UInventoryData;
class APlayerState;

// Lives on the PlayerState so it survives the pawn (decision D5, Doc B19 3.2).
// Owns the Personal Inventory; Section 7 adds the reservation ledger; Section 9 adds the bag state.
UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UPlayerInventoryComponent : public UActorComponent{
    GENERATED_BODY()

public:
    UPlayerInventoryComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Query. Null when the component is missing.
    static UPlayerInventoryComponent* findFor(const APlayerState* player);

    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory")
    UInventoryData* getPersonalInventory() const;

protected:
    // Replicated to the owner only (R01 §7). Other players never see this player's bag.
    UPROPERTY(ReplicatedUsing = OnRep_personalInventory)
    TObjectPtr<UInventoryData> personalInventory;

    UFUNCTION()
    void OnRep_personalInventory();

    // Server. Creates, initializes and registers the Personal Inventory as an owner-only subobject.
    void createPersonalInventory();
};
