#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DronePrototypeGameMode.generated.h"

/**
 * 기존 Third Person 기본 설정을 건드리지 않고 Drone Prototype Map에만 적용하는 GameMode.
 * native Class는 안전한 기본값을 제공하고, BP_DronePrototypeGameMode에서 Pawn·Controller의
 * Blueprint 자식 Class를 선택해 Asset 연결과 화면 외형을 교체한다.
 */
UCLASS(Blueprintable)
class ADronePrototypeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADronePrototypeGameMode();
};
