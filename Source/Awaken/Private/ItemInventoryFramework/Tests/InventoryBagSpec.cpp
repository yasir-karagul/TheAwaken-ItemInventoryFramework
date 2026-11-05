void FInventoryBagSpec::Define(){
    It("detach then reattach loses and duplicates nothing", [this](){
        FInventoryTestFixture fixture;
        fixture.addStackable(TEXT("Stick"), 10);
        fixture.addInstanced(TEXT("Spear"), 5, 50.0f);
        AInventoryTestPlayerState* player = fixture.makePlayer();
        UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
        const FGuid character = FGuid::NewGuid();
        component->setCharacterId(character);
        UInventoryData* bag = component->getPersonalInventory();
        bag->addStack(TEXT("Stick"), 4);
        FS_InventoryEntry spear;
        spear.itemId = TEXT("Spear");
        spear.instanceId = FGuid::NewGuid();
        spear.state.durability = 30.0f;
        bag->addInstance(spear);

        const FS_BagDetachResult detached = UBFL_InventoryBag::detachBag(player, FVector::ZeroVector);
        TestTrue(TEXT("detached"), detached.succeeded());
        TestTrue(TEXT("player is Bagless"), !UBFL_InventoryBag::hasBag(player));

        TestTrue(TEXT("recovered"), UBFL_InventoryBag::reattachBag(player, detached.bagId).succeeded());
        UInventoryData* restored = component->getPersonalInventory();
        TestEqual(TEXT("sticks equal"), restored->getTotalQuantity(TEXT("Stick")), 4);
        TestTrue(TEXT("same spear instance, same durability"), restored->findInstance(spear.instanceId) != nullptr && restored->findInstance(spear.instanceId)->state.durability == 30.0f);
        TestEqual(TEXT("exactly one spear"), restored->getInstanceCount(TEXT("Spear")), 1);
    });
    It("a failure at step 1 or 2 leaves the player with the bag and no stray record", [this](){
        FInventoryTestFixture fixture;
        AInventoryTestPlayerState* player = fixture.makePlayer();
        IConsoleVariable* injection = IConsoleManager::Get().FindConsoleVariable(TEXT("Awaken.Inventory.FailBagStep"));
        injection->Set(1);
        TestTrue(TEXT("step 1 refused"), UBFL_InventoryBag::detachBag(player, FVector::ZeroVector).failReason == T_Inventory_FailReason_BagActorFailed);
        TestTrue(TEXT("still has the bag"), UBFL_InventoryBag::hasBag(player));
        injection->Set(2);
        TestTrue(TEXT("step 2 refused"), UBFL_InventoryBag::detachBag(player, FVector::ZeroVector).failReason == T_Inventory_FailReason_GearReleaseFailed);
        TestTrue(TEXT("still has the bag"), UBFL_InventoryBag::hasBag(player));
        injection->Set(0);
    });
}
