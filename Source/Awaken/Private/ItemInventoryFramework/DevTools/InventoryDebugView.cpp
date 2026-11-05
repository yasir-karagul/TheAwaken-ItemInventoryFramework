#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "ItemInventoryFramework/UPlayerInventoryComponent.h"
#include "ItemInventoryFramework/UHotbarComponent.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/UInventoryRegistrySubsystem.h"

namespace{
    TAutoConsoleVariable<int32> cvarDebugInventory(TEXT("Awaken.Debug.Inventory"), 0, TEXT("1 = draw bag entries, reserved quantities, spoil times and registry size on screen."), ECVF_Default);
    FTSTicker::FDelegateHandle tickerHandle;

    bool drawInventoryDebug(float deltaTime){
        if(GEngine == nullptr){
            return true;
        }
        int32 messageKey = 9100;
        for(const FWorldContext& context : GEngine->GetWorldContexts()){
            const UWorld* world = context.World();
            if(world == nullptr || world->GetGameState() == nullptr){
                continue;
            }
            const UInventoryRegistrySubsystem* registry = world->GetSubsystem<UInventoryRegistrySubsystem>();
            GEngine->AddOnScreenDebugMessage(messageKey++, 0.3f, FColor::Cyan, FString::Printf(TEXT("[Inventory] registry holds %d inventories"), registry != nullptr ? registry->getAll().Num() : 0));
            for(const APlayerState* player : world->GetGameState()->PlayerArray){
                const UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
                const UInventoryData* bag = component != nullptr ? component->getPersonalInventory() : nullptr;
                if(bag == nullptr){
                    GEngine->AddOnScreenDebugMessage(messageKey++, 0.3f, FColor::Yellow, FString::Printf(TEXT("%s: Bagless"), *player->GetPlayerName()));
                    continue;
                }
                GEngine->AddOnScreenDebugMessage(messageKey++, 0.3f, FColor::Green, FString::Printf(TEXT("%s: %d entries, ledger %s"), *player->GetPlayerName(), bag->getEntries().Num(), component->validateLedger() ? TEXT("OK") : TEXT("MISMATCH")));
                for(const FS_InventoryEntry& entry : bag->getEntries()){
                    GEngine->AddOnScreenDebugMessage(messageKey++, 0.3f, FColor::White, FString::Printf(TEXT("   %s total %d reserved %d spoilAt %lld %s"), *entry.itemId.ToString(), entry.totalQuantity, entry.reservedQuantity, entry.spoilAtTime, entry.isInstanced() ? *entry.instanceId.ToString().Left(8) : TEXT("")));
                }
            }
        }
        return true;
    }

    void onDebugChanged(IConsoleVariable* variable){
        if(variable->GetInt() != 0 && !tickerHandle.IsValid()){
            tickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&drawInventoryDebug), 0.25f);   // short-lived: only while the view is on (Doc02 §5)
        }else if(variable->GetInt() == 0 && tickerHandle.IsValid()){
            FTSTicker::GetCoreTicker().RemoveTicker(tickerHandle);
            tickerHandle.Reset();
        }
    }

    struct FInventoryDebugRegistrar{
        FInventoryDebugRegistrar(){
            cvarDebugInventory->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&onDebugChanged));
        }
    };
    FInventoryDebugRegistrar registrar;
}

#endif
