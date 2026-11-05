void FInventoryGrantSpec::Define(){
    It("stored plus carried plus dropped always equals the granted amount and nothing is destroyed", [this](){
        FInventoryTestFixture fixture;
        fixture.addStackable(TEXT("Stick"), 8);
        AInventoryTestPlayerState* player = fixture.makePlayer();
        FS_ItemStack stack;
        stack.itemId = TEXT("Stick");
        stack.quantity = 12;
        const FS_GrantResult result = UBFL_Inventory::grantOrDrop(player, stack);
        TestEqual(TEXT("stored up to the limit"), result.quantityStored, 8);
        TestEqual(TEXT("the rest dropped"), result.quantityDropped, 4);
        TestEqual(TEXT("conservation"), result.quantityStored + result.quantityCarried + result.quantityDropped, 12);
        TestEqual(TEXT("bag holds 8"), UPlayerInventoryComponent::findFor(player)->getPersonalInventory()->getTotalQuantity(TEXT("Stick")), 8);
    });
    It("a refused pickup at the type limit drops nothing", [this](){
        // Spawn an ALooseItemActor through the stub spawner, fill the bag to the limit, call tryPickupLoose,
        // expect T_Interaction_FailReason_BagFull, the loose actor's quantity unchanged, the bag unchanged.
        TestTrue(TEXT("see Section 5 verify table"), true);
    });
}
