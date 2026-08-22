#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DronePrototypePlayerController.generated.h"

class UDroneFlightHUDWidget;

/** Local UI owner used only by the isolated Drone prototype. */
UCLASS()
class ADronePrototypePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADronePrototypePlayerController();

	UDroneFlightHUDWidget* GetFlightHUDWidget() const { return FlightHUDWidget; }
	TSubclassOf<UDroneFlightHUDWidget> GetFlightHUDWidgetClass() const { return FlightHUDWidgetClass; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly, Category="Prototype|UI", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UDroneFlightHUDWidget> FlightHUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDroneFlightHUDWidget> FlightHUDWidget;

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* PreviousPawn, APawn* NewPawn);

	void CreateFlightHUD();
	void SyncFlightHUDToPawn(APawn* NewPawn);
};
