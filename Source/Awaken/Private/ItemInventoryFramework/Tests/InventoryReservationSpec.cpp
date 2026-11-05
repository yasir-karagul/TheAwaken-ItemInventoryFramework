BEGIN_DEFINE_SPEC(FInventoryReservationSpec, "Awaken.Inventory.Reservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FInventoryReservationSpec)

void FInventoryReservationSpec::Define(){
    It("a reserved amount cannot be spent twice and a failed craft releases it", [this](){
        FInventoryTestFixture fixture;
        fixture.addStackable(TEXT("Stick"), 10);
        AInventoryTestPlayerState* player = fixture.makePlayer();
        UPlayerInventoryComponent* component = UPlayerInventoryComponent::findFor(player);
        UInventoryData* bag = component->getPersonalInventory();
        bag->addStack(TEXT("Stick"), 5);
        const FGuid session = FGuid::NewGuid();

        TestTrue(TEXT("reserve 3"), component->reserveForStaging(TEXT("Stick"), FGuid(), 3, session).succeeded());
        TestEqual(TEXT("available is 2"), bag->getAvailableQuantity(TEXT("Stick")), 2);
        TestEqual(TEXT("total is still 5"), bag->getTotalQuantity(TEXT("Stick")), 5);
        TestTrue(TEXT("reserving 3 more is refused"), !component->reserveForStaging(TEXT("Stick"), FGuid(), 3, session).succeeded());
        TestTrue(TEXT("removing reserved units is refused"), bag->removeStack(TEXT("Stick"), 4).failReason == T_Inventory_FailReason_ReservedQuantity);
        TestTrue(TEXT("ledger matches"), component->validateLedger());

        TestEqual(TEXT("released 3"), component->releaseStagedReservation(session), 3);
        TestEqual(TEXT("available back to 5"), bag->getAvailableQuantity(TEXT("Stick")), 5);
        TestTrue(TEXT("ledger empty and consistent"), component->validateLedger());
    });
}
