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

protected:
    UFUNCTION()
    void handlePersonalInventoryChanged(const FGuid& inventoryId);

    // Server. Re-binds the change handler; call whenever personalInventory is created or replaced.
    void bindPersonalInventory();

public:
    // Server. Asks the pawn's held item to re-check itself.
    void revalidateHeldItem() const;
```



public:
    // Server. CharacterID is a Doc R13 / B05 field (designer-approved); until the PlayerState carries it, it lives here.
    void setCharacterId(const FGuid& newCharacterId);
    FGuid getCharacterId() const;
    bool isStartingKitGranted() const;
    void markStartingKitGranted();

    // Server. Hands the Personal Inventory object out (bag detach). Returns it; the component keeps nothing.
    UInventoryData* detachPersonalInventory(UObject* newOuter);
    // Server. Takes an inventory object back as the Personal Inventory (bag recovery / fresh character).
    void attachPersonalInventory(UInventoryData* inventory);
    // Server. Fresh character: new empty bag, new CharacterID, kit not yet granted.
    void resetForNewCharacter(const FGuid& newCharacterId);

protected:
    FGuid characterId;               // server only
    bool startingKitGranted = false; // server only
```


};
