#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InventoryServices.generated.h"

UCLASS()
class AWAKEN_API UInventoryServices : public UGameInstanceSubsystem{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static UInventoryServices* get(const UObject* worldContext);
};
