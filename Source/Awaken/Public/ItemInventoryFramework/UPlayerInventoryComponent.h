#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"\n#include "ItemInventoryFramework/03Interfaces/BPI_SaveParticipantStub.h"
#include "UPlayerInventoryComponent.generated.h"

class UInventoryData;
class APlayerState;

// Lives on the PlayerState so it survives the pawn (decision D5, Doc B19 3.2).
// Owns the Personal Inventory; Section 7 adds the reservation ledger; Section 9 adds the bag state.
UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UPlayerInventoryComponent : public UActorComponent, public IBPI_SaveParticipant{
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
    void bindPersonalInventory();

public:
    void revalidateHeldItem() const;
    FS_InventoryResult reserveForStaging(FName itemId, const FGuid& instanceId, int32 quantity, const FGuid& sessionId);
    FS_InventoryResult releaseStagedItem(FName itemId, const FGuid& instanceId, int32 quantity, const FGuid& sessionId);
    int32 releaseStagedReservation(const FGuid& sessionId);
    int32 releaseAllReservations();
    FS_InventoryResult consumeReserved(const FGuid& sessionId, FName itemId, const FGuid& instanceId, int32 quantity);
    void forgetReservationsForItem(FName itemId);
    FGuid findMostWornAvailableInstance(FName itemId) const;
    bool validateLedger() const;
    void setCharacterId(const FGuid& newCharacterId);
    FGuid getCharacterId() const;
    bool isStartingKitGranted() const;
    void markStartingKitGranted();
    UInventoryData* detachPersonalInventory(UObject* newOuter);
    void attachPersonalInventory(UInventoryData* inventory);
    void resetForNewCharacter(const FGuid& newCharacterId);
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual FName getSaveKey_Implementation() override;
    virtual E_SaveScope getSaveScope_Implementation() override;
    virtual int32 getSnapshotVersion_Implementation() override;
    virtual TArray<uint8> getSnapshotForSave_Implementation() override;
    virtual bool restoreFromSnapshot_Implementation(const TArray<uint8>& snapshot) override;

protected:
    FGuid characterId;
    bool startingKitGranted = false;
    UPROPERTY() TArray<FS_StagedReservation> reservations;
};
