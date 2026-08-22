#include "Prototype/DronePrototypePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Drone.h"
#include "GameFramework/Pawn.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "UI/DroneFlightHUDWidget.h"

ADronePrototypePlayerController::ADronePrototypePlayerController()
{
	FlightHUDWidgetClass = UDroneFlightHUDWidget::StaticClass();
}

void ADronePrototypePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	OnPossessedPawnChanged.AddUniqueDynamic(
		this,
		&ADronePrototypePlayerController::HandlePossessedPawnChanged);
	CreateFlightHUD();
	SyncFlightHUDToPawn(GetPawn());
}

void ADronePrototypePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnPossessedPawnChanged.RemoveDynamic(
		this,
		&ADronePrototypePlayerController::HandlePossessedPawnChanged);

	if (FlightHUDWidget)
	{
		FlightHUDWidget->ClearTelemetrySource();
		FlightHUDWidget->RemoveFromParent();
		FlightHUDWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ADronePrototypePlayerController::HandlePossessedPawnChanged(APawn* /*PreviousPawn*/, APawn* NewPawn)
{
	SyncFlightHUDToPawn(NewPawn);
}

void ADronePrototypePlayerController::CreateFlightHUD()
{
	if (FlightHUDWidget || !FlightHUDWidgetClass)
	{
		return;
	}

	FlightHUDWidget = CreateWidget<UDroneFlightHUDWidget>(this, FlightHUDWidgetClass);
	if (!FlightHUDWidget)
	{
		UE_LOG(LogDrone, Error, TEXT("Prototype PlayerController could not create its Flight HUD Widget."));
		return;
	}

	if (!FlightHUDWidget->AddToPlayerScreen(10))
	{
		UE_LOG(LogDrone, Error, TEXT("Prototype PlayerController could not add its Flight HUD Widget to the local Player screen."));
		FlightHUDWidget->ClearTelemetrySource();
		FlightHUDWidget = nullptr;
	}
}

void ADronePrototypePlayerController::SyncFlightHUDToPawn(APawn* NewPawn)
{
	if (!FlightHUDWidget)
	{
		return;
	}

	UDroneTelemetryComponent* Telemetry = NewPawn
		? NewPawn->FindComponentByClass<UDroneTelemetryComponent>()
		: nullptr;
	FlightHUDWidget->SetTelemetrySource(Telemetry);
}
