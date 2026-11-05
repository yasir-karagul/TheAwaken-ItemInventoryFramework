#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "ItemInventoryFramework/UBFL_Inventory.h"
#include "ItemInventoryFramework/UBFL_InventoryBag.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UInventoryLayoutSubsystem.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"

namespace{
    FGuid lastDetachedBagId;

    // Optional last argument "p<N>" picks player N (default 0). Returns null with a log line when not on the host.
    APlayerState* resolvePlayer(UWorld* world, const TArray<FString>& arguments){
        if(world == nullptr || !UBFL_Inventory::isServerWorld(world)){
            INVENTORY_LOG(Warning, world, "Host command: refused on a client (the Doc B22 cheat manager forwards it)");
            return nullptr;
        }
        int32 playerIndex = 0;
        for(const FString& argument : arguments){
            if(argument.StartsWith(TEXT("p"))){
                playerIndex = FCString::Atoi(*argument.Mid(1));
            }
        }
        const AGameStateBase* gameState = world->GetGameState();
        if(gameState == nullptr || !gameState->PlayerArray.IsValidIndex(playerIndex)){
            INVENTORY_LOG(Warning, world, "No player %d", playerIndex);
            return nullptr;
        }
        return gameState->PlayerArray[playerIndex];
    }

    void giveCommand(const TArray<FString>& arguments, UWorld* world){
        APlayerState* player = resolvePlayer(world, arguments);
        if(player == nullptr || arguments.Num() < 2){
            INVENTORY_LOG(Log, world, "Usage: Awaken.Item.Give <itemId> <amount> [pN]");
            return;
        }
        FS_ItemStack stack;
        stack.itemId = FName(*arguments[0]);
        stack.quantity = FCString::Atoi(*arguments[1]);
        const FS_GrantResult result = UBFL_Inventory::grantOrDrop(player, stack);   // through GrantOrDrop, so a full bag drops (Doc02 §19.5)
        INVENTORY_LOG(Log, world, "Give %s x%d: stored %d, carried %d, dropped %d, reason %s", *stack.itemId.ToString(), stack.quantity, result.quantityStored, result.quantityCarried, result.quantityDropped, *result.failReason.ToString());
    }

    void fillBagCommand(const TArray<FString>& arguments, UWorld* world){
        // S13.2: builds the likeliest failure, a full bag: every bag-eligible item to its per-type limit.
        APlayerState* player = resolvePlayer(world, arguments);
        const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(world);
        if(player == nullptr || catalog == nullptr){
            return;
        }
        for(const TPair<FName, TObjectPtr<UDA_ItemDefinition>>& pair : catalog->getAllDefinitions()){
            const UDA_ItemDefinition* definition = pair.Value;
            if(definition != nullptr && !definition->isHeavy() && !definition->isPermanent() && !definition->isPortableContainer() && definition->perTypeLimit > 0){
                FS_ItemStack stack;
                stack.itemId = definition->itemId;
                stack.quantity = definition->perTypeLimit;
                UBFL_Inventory::grantOrDrop(player, stack);
            }
        }
        INVENTORY_LOG(Log, world, "FillBag done: every bag-eligible item is at its limit");
    }

    void spawnLooseCommand(const TArray<FString>& arguments, UWorld* world){
        APlayerState* player = resolvePlayer(world, arguments);
        const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(world);
        const UDA_ItemDefinition* definition = (catalog != nullptr && arguments.Num() > 0) ? catalog->getDefinition(FName(*arguments[0])) : nullptr;
        UInventoryServices* services = UInventoryServices::get(world);
        if(player == nullptr || definition == nullptr || services == nullptr){
            INVENTORY_LOG(Log, world, "Usage: Awaken.Item.SpawnLoose <itemId> [qty] [pN]");
            return;
        }
        FS_InventoryEntry entry = definition->itemKind == E_ItemKind::Instanced ? UBFL_Inventory::makeFreshInstance(player, *definition) : FS_InventoryEntry();
        entry.itemId = definition->itemId;
        entry.totalQuantity = definition->itemKind == E_ItemKind::Instanced ? 1 : (arguments.Num() > 1 && !arguments[1].StartsWith(TEXT("p")) ? FCString::Atoi(*arguments[1]) : 1);
        entry.spoilAtTime = UBFL_Inventory::computeSpoilAt(player, *definition);
        services->spawnLooseItem(player, entry, UBFL_Inventory::getDropLocation(player));
    }

    void clearBagCommand(const TArray<FString>& arguments, UWorld* world){
        // Keeps the Axe and the map: they are permanent (R01 §3.10, B22 S7-F22).
        APlayerState* player = resolvePlayer(world, arguments);
        UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
        const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(world);
        if(component == nullptr || !component->hasBag() || catalog == nullptr){
            return;
        }
        component->releaseAllReservations();
        UInventoryData* bag = component->getPersonalInventory();
        for(const FS_InventoryEntry& entry : bag->getEntriesCopy()){
            const UDA_ItemDefinition* definition = catalog->getDefinition(entry.itemId);
            if(definition == nullptr || definition->isPermanent()){
                continue;
            }
            if(entry.isInstanced()){
                bag->removeInstance(entry.instanceId);
            }else{
                bag->removeStack(entry.itemId, entry.totalQuantity);
            }
        }
        INVENTORY_LOG(Log, world, "ClearBag done (permanent items kept)");
    }

    void dropHereCommand(const TArray<FString>& arguments, UWorld* world){
        APlayerState* player = resolvePlayer(world, arguments);
        if(player == nullptr){
            return;
        }
        const FVector location = player->GetPawn() != nullptr ? player->GetPawn()->GetActorLocation() : FVector::ZeroVector;
        const FS_BagDetachResult result = UBFL_InventoryBag::detachBag(player, location);
        lastDetachedBagId = result.bagId;
        INVENTORY_LOG(Log, world, "DropHere: bag %s, reason %s", *result.bagId.ToString(), *result.failReason.ToString());
    }

    void setBaglessCommand(const TArray<FString>& arguments, UWorld* world){
        // Forces the Bagless state, optionally with the bag lying at x y z, to check the recovery rules.
        APlayerState* player = resolvePlayer(world, arguments);
        if(player == nullptr){
            return;
        }
        FVector location = player->GetPawn() != nullptr ? player->GetPawn()->GetActorLocation() : FVector::ZeroVector;
        if(arguments.Num() >= 3){
            location = FVector(FCString::Atof(*arguments[0]), FCString::Atof(*arguments[1]), FCString::Atof(*arguments[2]));
        }
        const FS_BagDetachResult result = UBFL_InventoryBag::detachBag(player, location);
        lastDetachedBagId = result.bagId;
        INVENTORY_LOG(Log, world, "SetBagless: bag %s, reason %s", *result.bagId.ToString(), *result.failReason.ToString());
    }

    void recoverBagCommand(const TArray<FString>& arguments, UWorld* world){
        APlayerState* player = resolvePlayer(world, arguments);
        if(player == nullptr){
            return;
        }
        const FS_InventoryResult result = UBFL_InventoryBag::reattachBag(player, lastDetachedBagId);
        INVENTORY_LOG(Log, world, "RecoverBag %s: %s", *lastDetachedBagId.ToString(), *result.failReason.ToString());
    }

    void seedCommand(const TArray<FString>& arguments, UWorld* world){
        UInventoryServices* services = UInventoryServices::get(world);
        if(services != nullptr && arguments.Num() > 0){
            services->setRandomSeed(FCString::Atoi(*arguments[0]));   // chance outcomes repeat (S8.8)
            INVENTORY_LOG(Log, world, "Seed set to %s", *arguments[0]);
        }
    }

    void validateCommand(const TArray<FString>& arguments, UWorld* world){
        TArray<FString> errors;
        const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(world);
        const UInventoryLayoutSubsystem* layout = UInventoryLayoutSubsystem::get(world);
        if(catalog != nullptr){
            catalog->validateAll(errors);
        }
        if(layout != nullptr){
            layout->validateLayout(errors);
        }
        for(const FString& error : errors){
            INVENTORY_LOG(Error, world, "Validate: %s", *error);
        }
        INVENTORY_LOG(Log, world, "Validate finished with %d errors", errors.Num());
    }

    FAutoConsoleCommandWithWorldAndArgs cmdGive(TEXT("Awaken.Item.Give"), TEXT("<itemId> <amount> [pN]: grant through GrantOrDrop"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&giveCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdFill(TEXT("Awaken.Inventory.FillBag"), TEXT("[pN]: fill every bag-eligible item to its limit (the full-bag situation)"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&fillBagCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdSpawn(TEXT("Awaken.Item.SpawnLoose"), TEXT("<itemId> [qty] [pN]: create a loose item in front of the player"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&spawnLooseCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdClear(TEXT("Awaken.Item.ClearBag"), TEXT("[pN]: empty the bag (keeps the Axe and the map)"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&clearBagCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdDrop(TEXT("Awaken.Bag.DropHere"), TEXT("[pN]: detach the bag at the player's feet"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&dropHereCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdBagless(TEXT("Awaken.Bag.SetBagless"), TEXT("[x y z] [pN]: force Bagless, bag lying at x y z"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&setBaglessCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdRecover(TEXT("Awaken.Bag.Recover"), TEXT("[pN]: recover the last detached bag"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&recoverBagCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdSeed(TEXT("Awaken.Inventory.Seed"), TEXT("<n>: seed the consume chance stream"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&seedCommand));
    FAutoConsoleCommandWithWorldAndArgs cmdValidate(TEXT("Awaken.Item.Validate"), TEXT(": cross-asset checks of the item catalog and the layouts"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&validateCommand));
}

#endif
