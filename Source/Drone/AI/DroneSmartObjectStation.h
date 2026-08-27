#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AI/DroneAITypes.h"
#include "DroneSmartObjectStation.generated.h"

class UArrowComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USmartObjectComponent;

/**
 * Patrol, Guard, Ambient, Cover, MG Turret를 Map에 배치하는 프로젝트 소유 Host Actor다.
 *
 * 이 Actor는 외형과 Smart Object Component 경계만 제공한다. 실제 Slot 수·Activity Tag·
 * Gameplay Interaction StateTree는 Smart Object Definition Asset에서 설정한다.
 */
UCLASS(Blueprintable)
class DRONE_API ADroneSmartObjectStation : public AActor
{
	GENERATED_BODY()

public:
	ADroneSmartObjectStation();

	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	USmartObjectComponent* GetSmartObjectComponent() const { return SmartObjectComponent; }

	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	USkeletalMeshComponent* GetStationMesh() const { return StationMesh; }

	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	UArrowComponent* GetSlotFacingPreview() const { return SlotFacingPreview; }

	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	EDroneSmartObjectActivity GetActivity() const { return Activity; }

	/** Definition의 Activity Tag에 넣어야 하는 Native Tag를 반환한다. */
	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	FGameplayTag GetExpectedActivityTag() const;

	UFUNCTION(BlueprintPure, Category="Drone|AI|SmartObject")
	bool HasSmartObjectDefinition() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|SmartObject|Components")
	TObjectPtr<USceneComponent> StationRoot;

	/** MG 외형이 필요할 때만 Mesh를 지정한다. Patrol/Ambient Point는 비워도 된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|SmartObject|Components")
	TObjectPtr<USkeletalMeshComponent> StationMesh;

	/** Smart Object Definition Asset을 이 Component의 Definition에 지정한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|SmartObject|Components")
	TObjectPtr<USmartObjectComponent> SmartObjectComponent;

	/** Editor에서 Slot의 위치와 +X 방향을 맞추기 위한 표시용 Component다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|SmartObject|Components")
	TObjectPtr<UArrowComponent> SlotFacingPreview;

	/** 문서·검증용 역할 값. 검색의 실제 기준은 Definition Activity Tag다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drone|AI|SmartObject")
	EDroneSmartObjectActivity Activity = EDroneSmartObjectActivity::EnemyPatrol;
};
