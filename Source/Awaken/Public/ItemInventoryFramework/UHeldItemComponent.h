#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UHeldItemComponent.generated.h"

class APawn;

// R01 §3.5, Doc B19 3.3: lives on the Character, NOT saved. Replicated to everyone so others see it in the hand.
UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UHeldItemComponent : public UActorComponent{
    GENERATED_BODY()

public:
    UHeldItemComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static UHeldItemComponent* findFor(const APawn* pawn);

    // ---- queries ----
    // Never empty: no item resolves to Unarmed (R01 §3.5).
    UFUNCTION(BlueprintPure, Category = "Awaken|Hotbar") FS_HeldItemView getHeldView() const;
    // What is drawn in the hand when it differs from what is wielded (Bagless hold slot).
    UFUNCTION(BlueprintPure, Category = "Awaken|Hotbar") FName getDisplayItemId() const;
    bool isInstanceHeld(const FGuid& instanceId) const;

    // ---- commands (server only) ----
    void setWielded(FName itemId, const FGuid& instanceId);
    void setDisplayOnly(FName itemId);
    void clearToUnarmed();
    // Drops back to Unarmed if the wielded thing is gone (broken, thrown, staged away, bag lost).
    void revalidate();

protected:
    UPROPERTY(Replicated) FName heldItemId;
    UPROPERTY(Replicated) FGuid heldInstanceId;
    UPROPERTY(Replicated) FName displayItemId;
};
