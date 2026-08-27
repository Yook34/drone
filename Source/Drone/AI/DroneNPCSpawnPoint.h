#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/DroneAITypes.h"
#include "DroneNPCSpawnPoint.generated.h"

class ADroneNPCCharacter;
class UArrowComponent;
class USceneComponent;

/**
 * Map에서 적/아군 NPC 초기 배치를 관리하는 안전한 Spawn Marker다.
 * Smart Object는 NPC 생성이 아니라 생성된 NPC의 순찰·대기·터렛 점유에 사용한다.
 */
UCLASS(Blueprintable)
class DRONE_API ADroneNPCSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ADroneNPCSpawnPoint();

	virtual void BeginPlay() override;

	/** bSpawnOnBeginPlay가 false일 때 Mission/Level Blueprint에서 명시적으로 호출한다. */
	UFUNCTION(BlueprintCallable, Category="Drone|AI|Spawn")
	int32 SpawnConfiguredNPCs();

	/** 이 Spawn Point가 만든 NPC만 파괴한다. Map에 직접 배치한 NPC는 건드리지 않는다. */
	UFUNCTION(BlueprintCallable, Category="Drone|AI|Spawn")
	void DestroySpawnedNPCs();

	UFUNCTION(BlueprintPure, Category="Drone|AI|Spawn")
	FTransform GetSpawnTransformForIndex(int32 Index) const;

	UFUNCTION(BlueprintPure, Category="Drone|AI|Spawn")
	int32 GetConfiguredSpawnCount() const { return SpawnCount; }

	UFUNCTION(BlueprintPure, Category="Drone|AI|Spawn")
	bool ShouldSpawnOnBeginPlay() const { return bSpawnOnBeginPlay; }

	UFUNCTION(BlueprintPure, Category="Drone|AI|Spawn")
	const FDroneNPCProfile& GetNPCProfile() const { return NPCProfile; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn|Components")
	TObjectPtr<USceneComponent> SpawnRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn|Components")
	TObjectPtr<UArrowComponent> SpawnDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn")
	TSubclassOf<ADroneNPCCharacter> NPCClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn", meta=(ShowOnlyInnerProperties))
	FDroneNPCProfile NPCProfile;

	/** 기본 false다. 배치만 했는데 게임이 바뀌는 일을 막고 Mission이 Spawn 시점을 소유한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn")
	bool bSpawnOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn", meta=(ClampMin="1", ClampMax="32"))
	int32 SpawnCount = 1;

	/** 여러 명일 때 Spawn Point의 좌우(Y축) 간격이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn", meta=(ClampMin="0.0"))
	float SpawnSpacing = 150.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Spawn|Runtime")
	TArray<TObjectPtr<ADroneNPCCharacter>> SpawnedNPCs;
};
