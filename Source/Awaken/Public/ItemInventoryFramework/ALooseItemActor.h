#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "ALooseItemActor.generated.h"

class UStaticMeshComponent;

// TEMPORARY STUB-LEVEL CLASS — owner: the Doc02 §19.7 loose-body owner (D4). When the shared base class exists,
// this becomes a thin subclass or is deleted. Callers only use IBPI_LooseItemSpawner.
UCLASS()
class AWAKEN_API ALooseItemActor : public AActor{
    GENERATED_BODY()

public:
    ALooseItemActor();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Command, server. Call between SpawnActorDeferred and FinishSpawning.
    void initializeFromEntry(const FS_InventoryEntry& newEntry);

    // Query, server. The whole entry (stack or instance), including spoil time and instance state.
    const FS_InventoryEntry& getEntry() const;
    // Command, server. Takes units away; destroys the actor at zero.
    void consumeUnits(int32 amount);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> meshComponent;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_itemId) FName itemId;
    UPROPERTY(Replicated) int32 quantity = 0;

    UFUNCTION()
    void OnRep_itemId();

    void applyMesh();

    FS_InventoryEntry entry;   // server only
};
