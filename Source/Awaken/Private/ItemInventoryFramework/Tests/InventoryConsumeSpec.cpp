void FInventoryConsumeSpec::Define(){
    It("fresh meat gives full hunger, spoiled gives half and always costs HP", [this](){
        FInventoryTestFixture fixture;
        UDA_ItemDefinition* meat = fixture.addSpoilable(TEXT("RawMeat"), TEXT("SpoiledMeat"), 24.0f);
        FS_ConsumeEffect hunger;
        hunger.kind = E_ConsumeEffectKind::MeterChange;
        hunger.tag = FGameplayTag::RequestGameplayTag(TEXT("Survival.Meter.Hunger"), false);
        hunger.amount = 100.0f;
        meat->consumeEffects.Add(hunger);
        meat->spoiledEffectiveness = 0.5f;
        UDA_ItemDefinition* spoiled = fixture.catalog()->getDefinition(TEXT("SpoiledMeat")) ? const_cast<UDA_ItemDefinition*>(fixture.catalog()->getDefinition(TEXT("SpoiledMeat"))) : nullptr;
        FS_ConsumeEffect loss;
        loss.kind = E_ConsumeEffectKind::HealthLoss;
        loss.amount = 10.0f;
        spoiled->consumeEffects.Add(loss);

        UTestSurvivalSink* sink = NewObject<UTestSurvivalSink>(fixture.gameInstance);
        fixture.services()->registerSeam(UBPI_SurvivalSink::StaticClass(), sink);
        AInventoryTestPlayerState* player = fixture.makePlayer();
        UInventoryData* bag = UPlayerInventoryComponent::findFor(player)->getPersonalInventory();
        bag->addStack(TEXT("RawMeat"), 2);

        TestTrue(TEXT("eat fresh"), UBFL_InventoryConsume::useConsumable(player, TEXT("RawMeat"), FGuid()).succeeded());
        TestEqual(TEXT("+100 hunger"), sink->meterDeltas.Last(), 100.0f);

        Cast<UStubGameClock>(fixture.services()->getSeam(UBPI_GameClock::StaticClass()))->gameSeconds = 25 * 3600;   // past 24 h
        TestTrue(TEXT("RawMeat is gone after the lazy conversion"), !UBFL_InventoryConsume::useConsumable(player, TEXT("RawMeat"), FGuid()).succeeded());
        TestEqual(TEXT("one unit became spoiled"), bag->getTotalQuantity(TEXT("SpoiledMeat")), 1);
        TestTrue(TEXT("eat spoiled"), UBFL_InventoryConsume::useConsumable(player, TEXT("SpoiledMeat"), FGuid()).succeeded());
        TestEqual(TEXT("half the hunger"), sink->meterDeltas.Last(), 50.0f);
    });
}
