#include "Prototype/DronePrototypeGameMode.h"

#include "Prototype/DronePrototypePawn.h"
#include "Prototype/DronePrototypePlayerController.h"

ADronePrototypeGameMode::ADronePrototypeGameMode()
{
	// native GameMode를 직접 선택하거나 자식 BP가 Class를 덮어쓰지 않았을 때 쓰는 기본값이다.
	// 실제 Map의 BP_DronePrototypeGameMode는 BP Pawn과 BP PlayerController로 이 값을 덮어쓴다.
	DefaultPawnClass = ADronePrototypePawn::StaticClass();
	PlayerControllerClass = ADronePrototypePlayerController::StaticClass();
}
