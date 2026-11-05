#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UInventoryRequestComponent.generated.h"

class APlayerState;
class ALooseItemActor;

UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UInventoryRequestComponent : public UActorComponent{
    GENERATED_BODY()

public:
    // The UI calls these on the owning client. They are RPCs, so on the host they run directly.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverTransfer(bool toContainer, FName itemId, FGuid instanceId, int32 quantity);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverBindHotbar(int32 slotIndex, FName itemId);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverSelectHotbar(int32 slotIndex);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverUseConsumable(FName itemId, FGuid instanceId);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverOpenContainer(FGuid containerId);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverCloseContainer();
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverDropItem(FName itemId, FGuid instanceId, int32 quantity);
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Awaken|Inventory") void serverPickupLoose(ALooseItemActor* loose, int32 quantity);
    // Reserve/release are NOT here: they are routed through Doc R02's crafting RPCs (R01 §7).

    // Server -> requesting client only. A refusal is never broadcast (Doc02 §6).
    UFUNCTION(Client, Reliable) void clientRefused(FGameplayTag reason);
    // Server -> owning client: Inventory.GrantDropped is "server, then local to the owning player" (S12.5).
    UFUNCTION(Client, Reliable) void clientGrantDropped(FName itemId, int32 quantity);

    static UInventoryRequestComponent* findFor(const APlayerState* player);

protected:
    APlayerState* getPlayerState() const;
    // Rate limit + player state + hands. Empty tag = allowed (Doc02 §19.6).
    FGameplayTag gate(bool needsHands, bool allowWhenIncapacitated);
    bool isValidQuantity(int32 quantity) const;
    bool isContainerReachable(APlayerState* player, const FGuid& containerId) const;
    void refuse(const FGameplayTag& reason, const TCHAR* operation);

    float tokens = 0.0f;
    double lastRefillTime = 0.0;
    bool bucketReady = false;
};
