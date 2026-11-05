#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventoryFramework/01Structs/S_ItemInventoryFramework.h"
#include "UContainerViewComponent.generated.h"

class UInventoryData;
class APlayerState;

UCLASS(ClassGroup = (Awaken), meta = (BlueprintSpawnableComponent))
class AWAKEN_API UContainerViewComponent : public UActorComponent{
    GENERATED_BODY()

public:
    UContainerViewComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static UContainerViewComponent* findFor(const APlayerState* player);

    // Command, server. Opens a view; closes the previous one first. Range is checked by the caller (Section 12).
    FS_InventoryResult openContainerView(const FGuid& containerId);
    // Command, server.
    void closeContainerView();

    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") UInventoryData* getViewMirror() const;
    UFUNCTION(BlueprintPure, Category = "Awaken|Inventory") FGuid getViewedContainerId() const;

protected:
    UPROPERTY(Replicated) TObjectPtr<UInventoryData> viewMirror;
    UPROPERTY(Replicated) FGuid viewedContainerId;

    UFUNCTION()
    void handleSourceChanged(const FGuid& changedId);

    TWeakObjectPtr<UInventoryData> viewedSource;
};
