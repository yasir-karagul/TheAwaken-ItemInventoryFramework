// TEMPORARY STUB — see InventoryStubs.h.
#include "ItemInventoryFramework/Stubs/InventoryStubs.h"
#include "ItemInventoryFramework/InventoryLog.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"

int64 UStubGameClock::getGameTimeSeconds_Implementation(){
    return gameSeconds;
}

void UStubSurvivalSink::modifyMeter_Implementation(APlayerState* player, FGameplayTag meterTag, float delta){
    INVENTORY_LOG(Log, this, "STUB modifyMeter %s %.2f", *meterTag.ToString(), delta);
}

void UStubSurvivalSink::registerIllnessSource_Implementation(APlayerState* player, FGameplayTag sourceTag, float durationHours){
    INVENTORY_LOG(Log, this, "STUB registerIllnessSource %s for %.2f h", *sourceTag.ToString(), durationHours);
}

bool UStubSurvivalSink::isMeterActive_Implementation(FGameplayTag meterTag){
    if(meterTag == T_Survival_Meter_Illness){
        return illnessActive;
    }
    return true;
}

void UStubVitalsSink::applyIllnessDamage_Implementation(APlayerState* player, float amount){
    INVENTORY_LOG(Log, this, "STUB applyIllnessDamage %.2f", amount);
}

void UStubVitalsSink::restoreHealth_Implementation(APlayerState* player, float amount){
    INVENTORY_LOG(Log, this, "STUB restoreHealth %.2f", amount);
}

bool UStubPlayerStateQuery::isIncapacitated_Implementation(APlayerState* player){
    return incapacitated;
}

bool UStubPlayerStateQuery::isCarryActive_Implementation(APlayerState* player){
    return carryActive;
}

bool UStubPlayerStateQuery::isInInteractRange_Implementation(APlayerState* player, AActor* target){
    return true;
}

bool UStubCarrySink::hasFreeCarrySlot_Implementation(APlayerState* player, int32 carryCost){
    return slotFree;
}

bool UStubCarrySink::beginCarry_Implementation(APlayerState* player, FName itemId){
    return slotFree;
}

FS_GearReleaseResult UStubEquipmentGear::releaseEquippedGear_Implementation(APlayerState* player){
    FS_GearReleaseResult result;
    result.succeeded = !failRelease;
    return result;
}

bool UStubEquipmentGear::reEquipGear_Implementation(APlayerState* player, const TArray<FS_GearSlotRecord>& gear){
    return true;
}

FGuid UStubBagActorService::createBagRecordAndActor_Implementation(APlayerState* player, FVector location, FGuid characterId, FGuid inventoryId){
    if(failCreate){
        return FGuid();
    }
    const FGuid bagId = FGuid::NewGuid();
    bagOwners.Add(bagId, characterId);
    bagLocations.Add(bagId, location);
    return bagId;
}

bool UStubBagActorService::deleteBag_Implementation(FGuid bagId){
    bagOwners.Remove(bagId);
    bagLocations.Remove(bagId);
    bagContents.Remove(bagId);
    return true;
}

void UStubBagActorService::finalizeBag_Implementation(FGuid bagId, const FS_BagContents& contents){
    bagContents.Add(bagId, contents);
}

bool UStubBagActorService::bagExists_Implementation(FGuid bagId){
    return bagOwners.Contains(bagId);
}

FS_BagContents UStubBagActorService::getBagContents_Implementation(FGuid bagId){
    const FS_BagContents* found = bagContents.Find(bagId);
    return found != nullptr ? *found : FS_BagContents();
}

FGuid UStubBagActorService::getBagOwner_Implementation(FGuid bagId){
    const FGuid* found = bagOwners.Find(bagId);
    return found != nullptr ? *found : FGuid();
}

FVector UStubBagActorService::getBagLocation_Implementation(FGuid bagId){
    const FVector* found = bagLocations.Find(bagId);
    return found != nullptr ? *found : FVector::ZeroVector;
}

void UStubBagActorService::removeBagsOwnedBy_Implementation(FGuid characterId){
    TArray<FGuid> toRemove;
    for(const TPair<FGuid, FGuid>& pair : bagOwners){
        if(pair.Value == characterId){
            toRemove.Add(pair.Key);
        }
    }
    for(const FGuid& bagId : toRemove){
        bagOwners.Remove(bagId);
        bagLocations.Remove(bagId);
        bagContents.Remove(bagId);
    }
}
