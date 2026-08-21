#include "Prototype/DronePrototypePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Drone.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Telemetry/DroneTelemetryComponent.h"

ADronePrototypePawn::ADronePrototypePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(45.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->SetCanEverAffectNavigation(false);
	SetRootComponent(CollisionComponent);

	VisualMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMeshComponent"));
	VisualMeshComponent->SetupAttachment(CollisionComponent);
	VisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMeshComponent->SetSimulatePhysics(false);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CollisionComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PrototypeMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PrototypeMovementComponent"));
	PrototypeMovementComponent->SetUpdatedComponent(CollisionComponent);
	PrototypeMovementComponent->MaxSpeed = 1200.0f;
	PrototypeMovementComponent->Acceleration = 2400.0f;
	PrototypeMovementComponent->Deceleration = 3000.0f;
	PrototypeMovementComponent->TurningBoost = 8.0f;

	TelemetryComponent = CreateDefaultSubobject<UDroneTelemetryComponent>(TEXT("TelemetryComponent"));
}

void ADronePrototypePawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	ApplyPrototypeMappingContext();
}

void ADronePrototypePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogDrone, Display, TEXT("Prototype pawn '%s' possessed by '%s'."), *GetNameSafe(this), *GetNameSafe(NewController));
}

void ADronePrototypePawn::UnPossessed()
{
	RemovePrototypeMappingContext();
	Super::UnPossessed();
}

UPawnMovementComponent* ADronePrototypePawn::GetMovementComponent() const
{
	return PrototypeMovementComponent;
}

void ADronePrototypePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemovePrototypeMappingContext();
	Super::EndPlay(EndPlayReason);
}

void ADronePrototypePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogDrone, Error, TEXT("Prototype pawn '%s' requires an Enhanced Input component."), *GetNameSafe(this));
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADronePrototypePawn::Move);
	}

	if (AltitudeAction)
	{
		EnhancedInputComponent->BindAction(AltitudeAction, ETriggerEvent::Triggered, this, &ADronePrototypePawn::ChangeAltitude);
	}

	if (YawAction)
	{
		EnhancedInputComponent->BindAction(YawAction, ETriggerEvent::Triggered, this, &ADronePrototypePawn::ChangeYaw);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADronePrototypePawn::Look);
	}

	if (CameraPitchRateAction)
	{
		EnhancedInputComponent->BindAction(
			CameraPitchRateAction,
			ETriggerEvent::Triggered,
			this,
			&ADronePrototypePawn::ChangeCameraPitch);
	}

	if (!MoveAction || !AltitudeAction || !YawAction || !LookAction || !CameraPitchRateAction)
	{
		UE_LOG(LogDrone, Display, TEXT("Prototype pawn '%s' does not have all prototype Input Actions assigned yet."), *GetNameSafe(this));
	}
}

void ADronePrototypePawn::ApplyPrototypeMappingContext()
{
	if (!PrototypeMappingContext)
	{
		UE_LOG(LogDrone, Display, TEXT("Prototype pawn '%s' has no prototype Input Mapping Context assigned yet."), *GetNameSafe(this));
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (bPrototypeMappingContextAdded && AppliedInputSubsystem.Get() == Subsystem
				&& AppliedMappingContext.Get() == PrototypeMappingContext
				&& Subsystem->HasMappingContext(PrototypeMappingContext))
			{
				return;
			}

			RemovePrototypeMappingContext();

			if (Subsystem->HasMappingContext(PrototypeMappingContext))
			{
				UE_LOG(LogDrone, Display, TEXT("Prototype mapping context for '%s' is already owned by another setup path."), *GetNameSafe(this));
				return;
			}

			Subsystem->AddMappingContext(PrototypeMappingContext, PrototypeMappingPriority);
			if (Subsystem->HasMappingContext(PrototypeMappingContext))
			{
				AppliedInputSubsystem = Subsystem;
				AppliedMappingContext = PrototypeMappingContext;
				bPrototypeMappingContextAdded = true;
			}
			else
			{
				UE_LOG(LogDrone, Warning, TEXT("Prototype mapping context for '%s' could not be registered."), *GetNameSafe(this));
			}
		}
	}
}

void ADronePrototypePawn::RemovePrototypeMappingContext()
{
	if (!bPrototypeMappingContextAdded)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = AppliedInputSubsystem.Get();
	UInputMappingContext* MappingContext = AppliedMappingContext.Get();
	if (Subsystem && MappingContext && Subsystem->HasMappingContext(MappingContext))
	{
		Subsystem->RemoveMappingContext(MappingContext);
	}

	AppliedInputSubsystem.Reset();
	AppliedMappingContext.Reset();
	bPrototypeMappingContextAdded = false;
}

void ADronePrototypePawn::Move(const FInputActionValue& Value)
{
	const FVector2D MovementValue = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MovementValue.Y);
	AddMovementInput(GetActorRightVector(), MovementValue.X);
}

void ADronePrototypePawn::ChangeAltitude(const FInputActionValue& Value)
{
	AddMovementInput(FVector::UpVector, Value.Get<float>());
}

void ADronePrototypePawn::ChangeYaw(const FInputActionValue& Value)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float YawDelta = Value.Get<float>() * PrototypeYawRateDegreesPerSecond * World->GetDeltaSeconds();
	AddActorLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void ADronePrototypePawn::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();
	AddActorLocalRotation(FRotator(0.0f, LookValue.X * PrototypeMouseYawDegreesPerInput, 0.0f));
	AdjustCameraPitch(LookValue.Y * PrototypeMousePitchDegreesPerInput);
}

void ADronePrototypePawn::ChangeCameraPitch(const FInputActionValue& Value)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float PitchDelta =
		Value.Get<float>() * PrototypeGamepadPitchRateDegreesPerSecond * World->GetDeltaSeconds();
	AdjustCameraPitch(PitchDelta);
}

void ADronePrototypePawn::AdjustCameraPitch(const float PitchDeltaDegrees)
{
	if (!CameraBoom || FMath::IsNearlyZero(PitchDeltaDegrees))
	{
		return;
	}

	FRotator BoomRotation = CameraBoom->GetRelativeRotation();
	BoomRotation.Pitch = FMath::Clamp(
		BoomRotation.Pitch + PitchDeltaDegrees,
		PrototypeMinimumCameraPitchDegrees,
		PrototypeMaximumCameraPitchDegrees);
	BoomRotation.Yaw = 0.0f;
	BoomRotation.Roll = 0.0f;
	CameraBoom->SetRelativeRotation(BoomRotation);
}
