#include "AI/DroneNPCAIController.h"

#include "AI/DroneAITags.h"
#include "AI/DroneNPCProfileComponent.h"
#include "AI/DroneSmartObjectReservationComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Prototype/DronePrototypePawn.h"

ADroneNPCAIController::ADroneNPCAIController()
{
	bAttachToPawn = true;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
	StateTreeAIComponent->SetStartLogicAutomatically(true);

	ReservationComponent = CreateDefaultSubobject<UDroneSmartObjectReservationComponent>(TEXT("ReservationComponent"));

	DronePerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("DronePerceptionComponent"));
	SetPerceptionComponent(*DronePerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	// 첫 감지 Spike용 편집 가능 시험값이다. 최종 탐지 거리·각도·난이도 규칙이 아니다.
	SightConfig->SightRadius = 4000.0f;
	SightConfig->LoseSightRadius = 4500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->SetMaxAge(3.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	DronePerceptionComponent->ConfigureSense(*SightConfig);
	DronePerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	DronePerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this,
		&ADroneNPCAIController::HandleTargetPerceptionUpdated);
}

void ADroneNPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const UDroneNPCProfileComponent* Profile = GetPossessedProfile())
	{
		ReservationComponent->SetUserTags(Profile->BuildSmartObjectUserTags());
	}
	ConfigureDefaultPatrolActivities();
}

void ADroneNPCAIController::OnUnPossess()
{
	ReservationComponent->ReleaseReservation();
	DetectedDrone.Reset();
	Super::OnUnPossess();
}

bool ADroneNPCAIController::UsesRifle() const
{
	const UDroneNPCProfileComponent* Profile = GetPossessedProfile();
	return Profile && Profile->GetProfile().WeaponType == EDroneNPCWeaponType::Rifle;
}

bool ADroneNPCAIController::UsesShotgun() const
{
	const UDroneNPCProfileComponent* Profile = GetPossessedProfile();
	return Profile && Profile->GetProfile().WeaponType == EDroneNPCWeaponType::Shotgun;
}

void ADroneNPCAIController::ConfigureDefaultPatrolActivities()
{
	FGameplayTagContainer Activities;
	const UDroneNPCProfileComponent* Profile = GetPossessedProfile();
	if (!Profile)
	{
		ReservationComponent->SetRequiredActivityTags(Activities);
		return;
	}

	if (Profile->IsHostile())
	{
		Activities.AddTag(DroneAITags::Activity_EnemyPatrol);
		Activities.AddTag(DroneAITags::Activity_Guard);
	}
	else if (Profile->IsFriendly())
	{
		Activities.AddTag(DroneAITags::Activity_FriendlyBasePatrol);
		Activities.AddTag(DroneAITags::Activity_Ambient);
	}
	else
	{
		Activities.AddTag(DroneAITags::Activity_Ambient);
	}

	ReservationComponent->SetRequiredActivityTags(Activities);
}

bool ADroneNPCAIController::PrepareMGTurretSearch()
{
	const UDroneNPCProfileComponent* Profile = GetPossessedProfile();
	if (!Profile || !Profile->IsHostile() || !Profile->GetProfile().bCanUseMGTurret)
	{
		return false;
	}

	FGameplayTagContainer Activities;
	Activities.AddTag(DroneAITags::Activity_MGTurret);
	ReservationComponent->SetRequiredActivityTags(Activities);
	return true;
}

void ADroneNPCAIController::HandleTargetPerceptionUpdated(AActor* Actor, const FAIStimulus Stimulus)
{
	// 현재 첫 감지 대상은 프로젝트 소유 Drone Prototype만 허용한다.
	if (!Actor || !Actor->IsA<ADronePrototypePawn>())
	{
		return;
	}

	const UDroneNPCProfileComponent* Profile = GetPossessedProfile();
	if (!Profile || !Profile->IsHostile())
	{
		// Friendly/Neutral NPC는 드론을 보더라도 전투 상태로 전환하지 않는다.
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		DetectedDrone = Actor;
		// 순찰·대기 Slot을 붙잡은 채 전투로 넘어가지 않도록 먼저 해제한다.
		ReservationComponent->ReleaseReservation();
		StateTreeAIComponent->SendStateTreeEvent(DroneAITags::Event_DroneDetected);
		OnDronePerceptionChanged.Broadcast(Actor, true);
	}
	else if (DetectedDrone.Get() == Actor)
	{
		DetectedDrone.Reset();
		StateTreeAIComponent->SendStateTreeEvent(DroneAITags::Event_DroneLost);
		OnDronePerceptionChanged.Broadcast(Actor, false);
	}
}

UDroneNPCProfileComponent* ADroneNPCAIController::GetPossessedProfile() const
{
	return GetPawn() ? GetPawn()->FindComponentByClass<UDroneNPCProfileComponent>() : nullptr;
}
