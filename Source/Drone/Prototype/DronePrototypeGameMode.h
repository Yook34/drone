#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DronePrototypeGameMode.generated.h"

/** GameMode used only by the isolated drone-control prototype. */
UCLASS()
class ADronePrototypeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADronePrototypeGameMode();
};
