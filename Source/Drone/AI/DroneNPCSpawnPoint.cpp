#include "AI/DroneNPCSpawnPoint.h"

#include "AI/DroneNPCCharacter.h"
#include "AI/DroneNPCProfileComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Drone.h"
#include "Engine/World.h"

ADroneNPCSpawnPoint::ADroneNPCSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnRoot"));
	SetRootComponent(SpawnRoot);

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetupAttachment(SpawnRoot);
	SpawnDirection->ArrowColor = FColor::Green;
	SpawnDirection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnDirection->SetCanEverAffectNavigation(false);
}

void ADroneNPCSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		SpawnConfiguredNPCs();
	}
}

int32 ADroneNPCSpawnPoint::SpawnConfiguredNPCs()
{
	SpawnedNPCs.RemoveAll([](const TObjectPtr<ADroneNPCCharacter>& NPC)
	{
		return !IsValid(NPC);
	});

	if (!SpawnedNPCs.IsEmpty())
	{
		UE_LOG(LogDrone, Warning, TEXT("Spawn point '%s' already owns active NPCs."), *GetNameSafe(this));
		return 0;
	}

	if (!NPCClass || !GetWorld())
	{
		UE_LOG(LogDrone, Warning, TEXT("Spawn point '%s' has no NPC Class."), *GetNameSafe(this));
		return 0;
	}

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const FTransform SpawnTransform = GetSpawnTransformForIndex(Index);
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		Params.bDeferConstruction = true;

		ADroneNPCCharacter* SpawnedNPC = GetWorld()->SpawnActor<ADroneNPCCharacter>(
			NPCClass,
			SpawnTransform,
			Params);
		if (!SpawnedNPC)
		{
			continue;
		}

		// Controller가 Possess하기 전에 Profile을 적용해 Friendly/Hostile Activity가 정확히 설정되게 한다.
		SpawnedNPC->GetNPCProfileComponent()->SetProfile(NPCProfile);
		SpawnedNPC->FinishSpawning(SpawnTransform);
		SpawnedNPCs.Add(SpawnedNPC);
		++SpawnedCount;
	}

	return SpawnedCount;
}

void ADroneNPCSpawnPoint::DestroySpawnedNPCs()
{
	for (ADroneNPCCharacter* NPC : SpawnedNPCs)
	{
		if (IsValid(NPC))
		{
			NPC->Destroy();
		}
	}
	SpawnedNPCs.Reset();
}

FTransform ADroneNPCSpawnPoint::GetSpawnTransformForIndex(const int32 Index) const
{
	const int32 SafeCount = FMath::Max(1, SpawnCount);
	const int32 SafeIndex = FMath::Clamp(Index, 0, SafeCount - 1);
	const float CenteredIndex = static_cast<float>(SafeIndex) - static_cast<float>(SafeCount - 1) * 0.5f;

	FTransform Result = GetActorTransform();
	Result.AddToTranslation(GetActorRightVector() * CenteredIndex * SpawnSpacing);
	return Result;
}
