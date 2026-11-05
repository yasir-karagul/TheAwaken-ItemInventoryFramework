// TEMPORARY STUB — owner: Doc B19.
UCLASS()
class AInventoryTestPlayerController : public APlayerController{
    GENERATED_BODY()

public:
    AInventoryTestPlayerController();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UInventoryRequestComponent> requestComponent;
};
