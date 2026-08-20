#include "Prototype/DronePrototypeGameMode.h"

#include "Prototype/DronePrototypePawn.h"

ADronePrototypeGameMode::ADronePrototypeGameMode()
{
	DefaultPawnClass = ADronePrototypePawn::StaticClass();
}
