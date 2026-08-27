#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneNPCNavigationFloor.generated.h"

class UBoxComponent;

/**
 * NPC Greybox 맵에서 시각 메시와 분리된 NavMesh 충돌 바닥.
 *
 * 구매 에셋이나 표시용 Static Mesh의 Collision 설정이 바뀌어도 AI 이동
 * 테스트가 독립적으로 재현되도록 UBoxComponent 하나만 제공한다.
 */
UCLASS(BlueprintType, Blueprintable)
class DRONE_API ADroneNPCNavigationFloor : public AActor
{
	GENERATED_BODY()

public:
	ADroneNPCNavigationFloor();

	UFUNCTION(BlueprintPure, Category = "Drone|AI|Navigation")
	UBoxComponent* GetNavigationCollision() const { return NavigationCollision; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone|AI|Navigation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> NavigationCollision;
};
