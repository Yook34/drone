#include "AI/DroneNPCNavigationFloor.h"

#include "AI/NavigationSystemBase.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

ADroneNPCNavigationFloor::ADroneNPCNavigationFloor()
{
	PrimaryActorTick.bCanEverTick = false;

	NavigationCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("NavigationCollision"));
	SetRootComponent(NavigationCollision);
	NavigationCollision->SetMobility(EComponentMobility::Static);
	NavigationCollision->SetBoxExtent(FVector(3200.0, 2600.0, 50.0));
	NavigationCollision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	NavigationCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NavigationCollision->SetCollisionObjectType(ECC_WorldStatic);
	NavigationCollision->SetCollisionResponseToAllChannels(ECR_Block);
	NavigationCollision->SetGenerateOverlapEvents(false);
	NavigationCollision->SetCanEverAffectNavigation(true);
	NavigationCollision->SetHiddenInGame(true);
}

void ADroneNPCNavigationFloor::BeginPlay()
{
	Super::BeginPlay();

	// PIE 월드 복제 시점에 생성된 Dynamic NavMesh에 이 바닥 충돌을
	// 명시적으로 다시 알린다. 이후 더티 영역 재빌드는 NavigationSystem이 담당한다.
	if (NavigationCollision)
	{
		FNavigationSystem::UpdateComponentData(*NavigationCollision);
	}
}
