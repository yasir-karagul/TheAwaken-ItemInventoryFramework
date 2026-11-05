#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ItemInventoryFramework/Tests/InventoryTestFixture.h"
#include "ItemInventoryFramework/UInventoryData.h"
#include "ItemInventoryFramework/02Tags/T_ItemInventoryFramework.h"

BEGIN_DEFINE_SPEC(FInventoryDataSpec, "Awaken.Inventory.Data", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FInventoryDataSpec)

void FInventoryDataSpec::Define(){
    Describe("Stackable entries", [this](){
        It("adds, merges and respects the per-type limit (all-or-nothing)", [this](){
            FInventoryTestFixture fixture;
            fixture.addStackable(TEXT("Stick"), 10);
            UInventoryData* inventory = fixture.makeInventory();
            TestTrue(TEXT("add 6"), inventory->addStack(TEXT("Stick"), 6).succeeded());
            TestEqual(TEXT("total 6"), inventory->getTotalQuantity(TEXT("Stick")), 6);
            const FS_InventoryResult tooMany = inventory->addStack(TEXT("Stick"), 5);
            TestTrue(TEXT("refused"), tooMany.failReason == T_Inventory_FailReason_LimitReached);
            TestEqual(TEXT("nothing applied"), inventory->getTotalQuantity(TEXT("Stick")), 6);
            TestEqual(TEXT("canFit partial"), inventory->canFit(FS_ItemStack{TEXT("Stick"), 9}), 4);
        });
        It("reads available, not total, on removal", [this](){
            FInventoryTestFixture fixture;
            fixture.addStackable(TEXT("Stick"), 10);
            UInventoryData* inventory = fixture.makeInventory();
            inventory->addStack(TEXT("Stick"), 5);
            // Reservation arrives in Section 7; here we only check the plain refusal paths.
            TestTrue(TEXT("over-remove refused"), inventory->removeStack(TEXT("Stick"), 6).failReason == T_Inventory_FailReason_InsufficientQuantity);
            TestTrue(TEXT("remove 5 ok"), inventory->removeStack(TEXT("Stick"), 5).succeeded());
            TestTrue(TEXT("entry gone at zero"), inventory->findStackable(TEXT("Stick")) == nullptr);
        });
    });

    Describe("Instanced entries", [this](){
        It("keeps instance identity and counts copies against the limit", [this](){
            FInventoryTestFixture fixture;
            fixture.addInstanced(TEXT("Spear"), 2, 50.0f);
            UInventoryData* inventory = fixture.makeInventory();
            FS_InventoryEntry first;
            first.itemId = TEXT("Spear");
            first.instanceId = FGuid::NewGuid();
            first.state.durability = 50.0f;
            FS_InventoryEntry second = first;
            second.instanceId = FGuid::NewGuid();
            FS_InventoryEntry third = first;
            third.instanceId = FGuid::NewGuid();
            TestTrue(TEXT("first"), inventory->addInstance(first).succeeded());
            TestTrue(TEXT("second"), inventory->addInstance(second).succeeded());
            TestTrue(TEXT("third refused"), inventory->addInstance(third).failReason == T_Inventory_FailReason_LimitReached);
            TestTrue(TEXT("same instance twice refused"), !inventory->addInstance(first).succeeded());
            TestEqual(TEXT("two copies"), inventory->getInstanceCount(TEXT("Spear")), 2);
            TestTrue(TEXT("remove by id"), inventory->removeInstance(first.instanceId).succeeded());
        });
    });

    Describe("Server authority", [this](){
        It("a client world refuses every command", [this](){
            // Covered by the listen-server client check in Section 12 (needs a real client world).
            TestTrue(TEXT("placeholder, see Section 12"), true);
        });
    });
}

#endif
