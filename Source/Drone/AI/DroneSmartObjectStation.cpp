#include "AI/DroneSmartObjectStation.h"

#include "AI/DroneAITags.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SmartObjectComponent.h"
#include "SmartObjectDefinition.h"

ADroneSmartObjectStation::ADroneSmartObjectStation()
{
	PrimaryActorTick.bCanEverTick = false;

	StationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StationRoot"));
	SetRootComponent(StationRoot);

	// 외형 Asset은 프로젝트 소유 BP에서 지정한다. C++는 ThirdParty 경로를 하드코딩하지 않는다.
	StationMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetupAttachment(StationRoot);
	StationMesh->SetSimulatePhysics(false);

	SmartObjectComponent = CreateDefaultSubobject<USmartObjectComponent>(TEXT("SmartObjectComponent"));
	SmartObjectComponent->SetupAttachment(StationRoot);

	// Definition Slot Transform을 배치할 때 Actor +X 방향을 확인하는 Editor 표식이다.
	SlotFacingPreview = CreateDefaultSubobject<UArrowComponent>(TEXT("SlotFacingPreview"));
	SlotFacingPreview->SetupAttachment(StationRoot);
	SlotFacingPreview->ArrowColor = FColor::Cyan;
	SlotFacingPreview->ArrowSize = 1.5f;
	SlotFacingPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlotFacingPreview->SetCanEverAffectNavigation(false);
}

void ADroneSmartObjectStation::SetSmartObjectDefinition(USmartObjectDefinition* Definition)
{
	SmartObjectComponent->SetDefinition(Definition);
}

USmartObjectDefinition* ADroneSmartObjectStation::GetSmartObjectDefinition() const
{
	return const_cast<USmartObjectDefinition*>(SmartObjectComponent->GetBaseDefinition());
}

void ADroneSmartObjectStation::SetStationSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
	StationMesh->SetSkeletalMeshAsset(SkeletalMesh);
}

USkeletalMesh* ADroneSmartObjectStation::GetStationSkeletalMesh() const
{
	return StationMesh->GetSkeletalMeshAsset();
}

FGameplayTag ADroneSmartObjectStation::GetExpectedActivityTag() const
{
	return DroneAITags::GetActivityTag(Activity);
}

bool ADroneSmartObjectStation::HasSmartObjectDefinition() const
{
	return SmartObjectComponent && SmartObjectComponent->GetBaseDefinition() != nullptr;
}
