#include "ItemInventoryFramework/InventoryServices.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/03Interfaces/BPI_GameClock.h"
#include "ItemInventoryFramework/03Interfaces/BPI_SurvivalSink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_VitalsSink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_PlayerStateQuery.h"
#include "ItemInventoryFramework/03Interfaces/BPI_CarrySink.h"
#include "ItemInventoryFramework/03Interfaces/BPI_EquipmentGear.h"
#include "ItemInventoryFramework/03Interfaces/BPI_LooseItemSpawner.h"
#include "ItemInventoryFramework/03Interfaces/BPI_BagActorService.h"
#include "ItemInventoryFramework/Stubs/InventoryStubs.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UInventoryServices::Initialize(FSubsystemCollectionBase& Collection){
    Super::Initialize(Collection);
    randomStream.Initialize(FMath::Rand());

    // Default every seam to its stub. A real framework replaces one with registerSeam().
    registerSeam(UBPI_GameClock::StaticClass(), NewObject<UStubGameClock>(this));
    registerSeam(UBPI_SurvivalSink::StaticClass(), NewObject<UStubSurvivalSink>(this));
    registerSeam(UBPI_VitalsSink::StaticClass(), NewObject<UStubVitalsSink>(this));
    registerSeam(UBPI_PlayerStateQuery::StaticClass(), NewObject<UStubPlayerStateQuery>(this));
    registerSeam(UBPI_CarrySink::StaticClass(), NewObject<UStubCarrySink>(this));
    registerSeam(UBPI_EquipmentGear::StaticClass(), NewObject<UStubEquipmentGear>(this));
    registerSeam(UBPI_LooseItemSpawner::StaticClass(), NewObject<UStubLooseItemSpawner>(this));
    registerSeam(UBPI_BagActorService::StaticClass(), NewObject<UStubBagActorService>(this));

    INVENTORY_LOG(Log, this, "Item & Inventory framework started with %d seams (stubs)", seams.Num());
}

UInventoryServices* UInventoryServices::get(const UObject* worldContext){
    UGameInstance* gameInstance = UGameplayStatics::GetGameInstance(worldContext);
    if(gameInstance == nullptr){
        return nullptr;
    }
    return gameInstance->GetSubsystem<UInventoryServices>();
}

bool UInventoryServices::registerSeam(UClass* interfaceClass, UObject* implementation){
    if(interfaceClass == nullptr || implementation == nullptr){
        return false;
    }
    if(!implementation->GetClass()->ImplementsInterface(interfaceClass)){
        INVENTORY_LOG(Error, this, "registerSeam: %s does not implement %s", *implementation->GetName(), *interfaceClass->GetName());
        return false;
    }
    seams.Add(interfaceClass, implementation);
    return true;
}

UObject* UInventoryServices::getSeam(const UClass* interfaceClass) const{
    const TObjectPtr<UObject>* found = seams.Find(interfaceClass);
    return found != nullptr ? found->Get() : nullptr;
}

int64 UInventoryServices::getGameTimeSeconds() const{
    UObject* clock = getSeam(UBPI_GameClock::StaticClass());
    return clock != nullptr ? IBPI_GameClock::Execute_getGameTimeSeconds(clock) : 0;
}

bool UInventoryServices::isIncapacitated(APlayerState* player) const{
    UObject* query = getSeam(UBPI_PlayerStateQuery::StaticClass());
    return query != nullptr && IBPI_PlayerStateQuery::Execute_isIncapacitated(query, player);
}

bool UInventoryServices::isCarryActive(APlayerState* player) const{
    UObject* query = getSeam(UBPI_PlayerStateQuery::StaticClass());
    return query != nullptr && IBPI_PlayerStateQuery::Execute_isCarryActive(query, player);
}

bool UInventoryServices::isInInteractRange(APlayerState* player, AActor* target) const{
    UObject* query = getSeam(UBPI_PlayerStateQuery::StaticClass());
    return query != nullptr && IBPI_PlayerStateQuery::Execute_isInInteractRange(query, player, target);
}

bool UInventoryServices::isMeterActive(const FGameplayTag& meterTag) const{
    UObject* sink = getSeam(UBPI_SurvivalSink::StaticClass());
    return sink != nullptr && IBPI_SurvivalSink::Execute_isMeterActive(sink, meterTag);
}

void UInventoryServices::modifyMeter(APlayerState* player, const FGameplayTag& meterTag, float delta) const{
    UObject* sink = getSeam(UBPI_SurvivalSink::StaticClass());
    if(sink != nullptr){
        IBPI_SurvivalSink::Execute_modifyMeter(sink, player, meterTag, delta);
    }
}

void UInventoryServices::registerIllnessSource(APlayerState* player, const FGameplayTag& sourceTag, float hours) const{
    UObject* sink = getSeam(UBPI_SurvivalSink::StaticClass());
    if(sink != nullptr){
        IBPI_SurvivalSink::Execute_registerIllnessSource(sink, player, sourceTag, hours);
    }
}

void UInventoryServices::applyIllnessDamage(APlayerState* player, float amount) const{
    UObject* sink = getSeam(UBPI_VitalsSink::StaticClass());
    if(sink != nullptr){
        IBPI_VitalsSink::Execute_applyIllnessDamage(sink, player, amount);
    }
}

void UInventoryServices::restoreHealth(APlayerState* player, float amount) const{
    UObject* sink = getSeam(UBPI_VitalsSink::StaticClass());
    if(sink != nullptr){
        IBPI_VitalsSink::Execute_restoreHealth(sink, player, amount);
    }
}

bool UInventoryServices::hasFreeCarrySlot(APlayerState* player, int32 carryCost) const{
    UObject* sink = getSeam(UBPI_CarrySink::StaticClass());
    return sink != nullptr && IBPI_CarrySink::Execute_hasFreeCarrySlot(sink, player, carryCost);
}

bool UInventoryServices::beginCarry(APlayerState* player, FName itemId) const{
    UObject* sink = getSeam(UBPI_CarrySink::StaticClass());
    return sink != nullptr && IBPI_CarrySink::Execute_beginCarry(sink, player, itemId);
}

FS_GearReleaseResult UInventoryServices::releaseEquippedGear(APlayerState* player) const{
    UObject* gear = getSeam(UBPI_EquipmentGear::StaticClass());
    if(gear == nullptr){
        return FS_GearReleaseResult();
    }
    return IBPI_EquipmentGear::Execute_releaseEquippedGear(gear, player);
}

bool UInventoryServices::reEquipGear(APlayerState* player, const TArray<FS_GearSlotRecord>& gearSlots) const{
    UObject* gear = getSeam(UBPI_EquipmentGear::StaticClass());
    return gear != nullptr && IBPI_EquipmentGear::Execute_reEquipGear(gear, player, gearSlots);
}

AActor* UInventoryServices::spawnLooseItem(UObject* worldContext, const FS_InventoryEntry& entry, FVector location) const{
    UObject* spawner = getSeam(UBPI_LooseItemSpawner::StaticClass());
    return spawner != nullptr ? IBPI_LooseItemSpawner::Execute_spawnLooseItem(spawner, worldContext, entry, location) : nullptr;
}

UObject* UInventoryServices::getBagService() const{
    return getSeam(UBPI_BagActorService::StaticClass());
}

void UInventoryServices::setRandomSeed(int32 seed){
    randomStream.Initialize(seed);
}

float UInventoryServices::rollChance(){
    return randomStream.FRand();
}
