#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAwakenInventory, Log, All);

namespace InventoryLog{
    AWAKEN_API FString roleName(const UObject* worldContext);
}

// Every line names the machine role (Host, Client, Offline). Use: INVENTORY_LOG(Log, this, "text %d", value);
#define INVENTORY_LOG(verbosity, worldContext, format, ...) \
    UE_LOG(LogAwakenInventory, verbosity, TEXT("[%s] " format), *InventoryLog::roleName(worldContext), ##__VA_ARGS__)
