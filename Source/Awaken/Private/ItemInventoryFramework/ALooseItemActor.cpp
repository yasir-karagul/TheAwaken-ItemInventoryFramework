#include "ItemInventoryFramework/ALooseItemActor.h"
#include "ItemInventoryFramework/UDA_ItemDefinition.h"
#include "ItemInventoryFramework/UItemCatalogSubsystem.h"
#include "ItemInventoryFramework/UItemInventorySettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

ALooseItemActor::ALooseItemActor(){
    meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(meshComponent);
    meshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(5.0f);   // low-rate position (Doc02 §19.7)
}

void ALooseItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALooseItemActor, itemId);
    DOREPLIFETIME(ALooseItemActor, quantity);
}

void ALooseItemActor::initializeFromEntry(const FS_InventoryEntry& newEntry){
    entry = newEntry;
    itemId = newEntry.itemId;
    quantity = newEntry.totalQuantity;
}

const FS_InventoryEntry& ALooseItemActor::getEntry() const{
    return entry;
}

void ALooseItemActor::BeginPlay(){
    Super::BeginPlay();
    applyMesh();
    if(HasAuthority()){
        meshComponent->SetSimulatePhysics(true);   // the server simulates; bodies sleep by engine defaults
        const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
        const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
        float lifetime = UItemInventorySettings::get()->defaultLooseLifetimeSeconds;
        if(definition != nullptr && definition->looseLifetimeSeconds > 0.0f){
            lifetime = definition->looseLifetimeSeconds;
        }
        SetLifeSpan(lifetime);   // Doc02 §19.7: lifetime in data, no global cap
    }
}

void ALooseItemActor::OnRep_itemId(){
    applyMesh();
}

void ALooseItemActor::applyMesh(){
    const UItemCatalogSubsystem* catalog = UItemCatalogSubsystem::get(this);
    const UDA_ItemDefinition* definition = catalog != nullptr ? catalog->getDefinition(itemId) : nullptr;
    if(definition != nullptr && !definition->worldMesh.IsNull()){
        // Synchronous load is acceptable for the temporary actor; the shared base class should stream it.
        meshComponent->SetStaticMesh(definition->worldMesh.LoadSynchronous());
    }
}

void ALooseItemActor::consumeUnits(int32 amount){
    if(!HasAuthority() || amount <= 0){
        return;
    }
    entry.totalQuantity = FMath::Max(0, entry.totalQuantity - amount);
    quantity = entry.totalQuantity;
    if(entry.totalQuantity <= 0){
        Destroy();
    }
}
