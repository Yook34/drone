#include "Prototype/DronePrototypeGameMode.h"

#include "Prototype/DronePrototypePawn.h"
#include "Prototype/DronePrototypePlayerController.h"

ADronePrototypeGameMode::ADronePrototypeGameMode()
{
	DefaultPawnClass = ADronePrototypePawn::StaticClass();
	PlayerControllerClass = ADronePrototypePlayerController::StaticClass();
}
