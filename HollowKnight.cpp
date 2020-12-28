// Fill out your copyright notice in the Description page of Project Settings.


#include "HollowKnight.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"

// Sets default values
AHollowKnight::AHollowKnight()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	capsule = CreateDefaultSubobject<USphereComponent>(TEXT("Knight_SphereComponent"));

	pawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Knight_PawnMovement"));

	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Knight_CameraComponent"));
}

// Called when the game starts or when spawned
void AHollowKnight::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* controller = GetWorld()->GetFirstPlayerController();

	controller->Possess(this);
	controller->SetViewTarget(this);

	camera->SetRelativeLocation(FVector(-1000, 0, 0));

	maxWalkSpeed = 800.f; // Original values: 800.f
	maxFallSpeed = -1000.f; // Original values: -1000.f
	wallFallSpeed = -300.f; // Original values: -300.f

	jumpAcceleration = 9000.f; // Original values: 7000.f
	jumpMaxDuration = .233333f; // Original values: .233333f
	jumpSpeedCap = 900.f; // Original values: 700.f
	doubleJump = false;

	dashDuration = .133333f; // Original values: ???
	dashAcceleration = 10000.f; // Original values: ???
	dashMaxSpeed = 2000.f; // Original values: ???
	dashCounter = 0;

	gravityAcceleration = 3000.f; // Original values: 3000.f
	risingGravityAcceleration = 2100.f; // Original values: 2100.f

	wallJumpInitial = false;

	ChangeMovement(EMotionStates::FALLING);

	capsule->SetSphereRadius(50.f);
	capsule->SetHiddenInGame(false);
	capsule->SetSimulatePhysics(true);
	capsule->SetEnableGravity(false);
	capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	capsule->OnComponentHit.AddDynamic(this, &AHollowKnight::OnHit);

}

// Called every frame
void AHollowKnight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddGravity();
	Jumping(DeltaTime);
	Dashing(DeltaTime);
	CheckWallCollision();

	ResolveMovement(DeltaTime);
}

// Called to bind functionality to input
void AHollowKnight::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	APlayerController* controller = GetWorld()->GetFirstPlayerController();
	PlayerInputComponent->BindAxis(FName("MoveHorizontal"), this, &AHollowKnight::MoveHorizontal);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &AHollowKnight::StartJump);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Released, this, &AHollowKnight::InterruptJump);
	PlayerInputComponent->BindAction(FName("Dash"), IE_Pressed, this, &AHollowKnight::StartDash);
}

void AHollowKnight::AddVelocity(FVector direction, float scale)
{
	this->addedVelocity += direction * scale;
}

void AHollowKnight::ResolveMovement(float deltaTime)
{
	if (motionState == EMotionStates::JUMPING || motionState == EMotionStates::RISING) {
		if (this->velocity.Z <= 0.01f) {
			ChangeMovement(EMotionStates::FALLING);
		}
	}
	FVector initialLocation = this->GetActorLocation();

	FVector totalVelocity = (FMath::Sign(this->addedVelocity.Y) == FMath::Sign(this->velocity.Y)) ? this->addedVelocity + this->velocity : this->addedVelocity;
	totalVelocity = FMath::Sign(this->velocity.Y) == 0 ? this->addedVelocity + this->velocity : totalVelocity;

	float horizontalClampMin = (motionState == EMotionStates::DASHING) ? -1 * this->dashMaxSpeed : -1 * this->maxWalkSpeed;
	float horizontalClampMax = (motionState == EMotionStates::DASHING) ? this->dashMaxSpeed : this->maxWalkSpeed;

	float verticalClampMin = (motionState == EMotionStates::GROUNDED || motionState == EMotionStates::FALLING) ? this->maxFallSpeed : 0.f;
	verticalClampMin = (motionState == EMotionStates::WALLSLIDE) ? this->wallFallSpeed : verticalClampMin;
	verticalClampMin = (motionState == EMotionStates::WALLSLIDE && this->landedOnWall) ? 0 : verticalClampMin;
	verticalClampMin = (motionState == EMotionStates::DASHING) ? 0 : verticalClampMin;

	float verticalClampMax = (motionState == EMotionStates::DASHING) ? 0 : this->jumpSpeedCap;

	totalVelocity = FVector(
		totalVelocity.X, 
		FMath::Clamp(totalVelocity.Y, horizontalClampMin, horizontalClampMax),
		FMath::Clamp(totalVelocity.Z, verticalClampMin, verticalClampMax));
	FVector newLocation = deltaTime * totalVelocity + initialLocation;

	FHitResult hitResult;
	SetActorLocation(newLocation, true, &hitResult, ETeleportType::None);
	while(hitResult.bBlockingHit) {
		FVector displacement = hitResult.TraceEnd - hitResult.TraceStart;
		FVector displacementAdjustment = displacement - FVector::DotProduct(displacement, hitResult.ImpactNormal) * hitResult.ImpactNormal + hitResult.ImpactNormal * 0.0125;
		FVector displacedActor = this->GetActorLocation() + displacementAdjustment;
		displacedActor = FVector(0.f, displacedActor.Y, displacedActor.Z);
		SetActorLocation(displacedActor, true, &hitResult, ETeleportType::None);
	}

	this->velocity = (this->GetActorLocation() - initialLocation) / deltaTime;
	this->addedVelocity = FVector::ZeroVector;
}

bool AHollowKnight::ChangeMovement(EMotionStates::Type newMotionState)
{
	EMotionStates::Type oldMotionState = motionState;

	switch (newMotionState)
	{
	case EMotionStates::GROUNDED:
		if (oldMotionState != EMotionStates::DASHING) {
			motionState = newMotionState;
		}
		break;
	case EMotionStates::JUMPING:
		if (oldMotionState != EMotionStates::DASHING) {
			motionState = newMotionState;
		}
		break;
	case EMotionStates::RISING:
		if (oldMotionState != EMotionStates::DASHING) {
			motionState = newMotionState;
		}
		break;
	case EMotionStates::FALLING:
		if (oldMotionState != EMotionStates::JUMPING) {
			motionState = newMotionState;
		}
		break;
	case EMotionStates::DASHING:
		motionState = newMotionState;
		break;
	case EMotionStates::WALLSLIDE:
		if (oldMotionState == EMotionStates::FALLING) {
			motionState = newMotionState;
		}
		break;
	}

	if (oldMotionState != newMotionState) {
		if (oldMotionState == EMotionStates::WALLSLIDE) {
			this->landedOnWall = false;
		}
		if (newMotionState == EMotionStates::WALLSLIDE) {
			this->landedOnWall = true;
			UKismetSystemLibrary::K2_SetTimer(this, TEXT("CompleteWallLanding"), .1f, false);
		}
	}

	return oldMotionState != newMotionState;
}

void AHollowKnight::AddGravity()
{
	float addedGravity = gravityAcceleration;
	switch (motionState)
	{
	case EMotionStates::GROUNDED:
		break;
	case EMotionStates::JUMPING:
		addedGravity = 0.f;
		break;
	case EMotionStates::RISING:
		addedGravity = risingGravityAcceleration;
		break;
	case EMotionStates::FALLING:
		break;
	case EMotionStates::WALLSLIDE:
		AddVelocity(FVector::RightVector, FMath::Sign(this->wallNormal.Y) * -1.f);
		break;
	}

	AddVelocity(FVector::DownVector, addedGravity);
}

void AHollowKnight::MoveHorizontal(float amount)
{
	if (!this->wallJumpInitial && motionState != EMotionStates::DASHING) {
		if (FMath::Abs(amount) != 0) {
			this->lookDirection = amount > 0 ? FVector::RightVector : FVector::LeftVector;
		}
		AddVelocity(amount * FVector::RightVector, maxWalkSpeed);
	}
}

void AHollowKnight::CheckWallCollision()
{
	FVector rightSphereCenter = this->GetActorLocation() + 50 * FVector::RightVector;
	FVector leftSphereCenter = this->GetActorLocation() + 50 * FVector::LeftVector;
	FCollisionShape collisionSphere = FCollisionShape::MakeSphere(5.0f);

	TArray<TEnumAsByte<EObjectTypeQuery>> collisionParams;
	collisionParams.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	collisionParams.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	TArray<AActor*> ignoredActors;
	FHitResult rightOutHit;
	FHitResult leftOutHit;
	bool rightHit = UKismetSystemLibrary::SphereTraceSingleForObjects(this, rightSphereCenter, rightSphereCenter, collisionSphere.GetSphereRadius(), 
		collisionParams, false, ignoredActors, EDrawDebugTrace::None, rightOutHit, true);
	bool leftHit = UKismetSystemLibrary::SphereTraceSingleForObjects(this, leftSphereCenter, leftSphereCenter, collisionSphere.GetSphereRadius(),
		collisionParams, false, ignoredActors, EDrawDebugTrace::None, leftOutHit, true);

	if (rightOutHit.bBlockingHit) {
		if (FMath::Abs(FVector::DotProduct(rightOutHit.ImpactNormal, FVector::RightVector)) >= 0.71f){
			this->wallDirection = FVector::RightVector * FMath::Sign(rightOutHit.ImpactNormal.Y);
		}
	}
	if (leftOutHit.bBlockingHit) {
		if (FMath::Abs(FVector::DotProduct(leftOutHit.ImpactNormal, FVector::LeftVector)) >= 0.71f) {
			this->wallDirection = FVector::RightVector * FMath::Sign(leftOutHit.ImpactNormal.Y);
		}
	}

	this->touchingWall = rightHit || leftHit;
}

void AHollowKnight::CompleteWallLanding()
{
	this->landedOnWall = false;
}

void AHollowKnight::StartJump()
{
	this->jumpTimer = 0;
	if (motionState == EMotionStates::GROUNDED) {
		ChangeMovement(EMotionStates::JUMPING);
	}
	else {
		if (this->touchingWall) {
			ChangeMovement(EMotionStates::JUMPING);
			this->wallJumpInitial = true;
			this->lookDirection = -1 * this->wallDirection;
			this->velocity = FVector::ZeroVector;
		}
		else {
			if (IsInAir() && this->doubleJump) {
				this->doubleJump = false;
				ChangeMovement(EMotionStates::JUMPING);
			}
		}
	}
}

void AHollowKnight::Jumping(float deltaTime)
{
	if (motionState == EMotionStates::JUMPING) {
		if (this->wallJumpInitial) {
			FVector wallJumpDirection = FVector::UpVector + this->wallDirection;
			AddVelocity(wallJumpDirection, this->jumpAcceleration);
		}
		else {
			AddVelocity(FVector::UpVector, this->jumpAcceleration);
		}

		this->jumpTimer += deltaTime;
		if (this->jumpTimer >= this->jumpMaxDuration) {
			this->jumpTimer = 0;
			StopJump(false);
		}

		if (this->jumpTimer >= this->jumpMaxDuration / 2) {
			this->wallJumpInitial = false;
		}
	}
}

void AHollowKnight::StopJump(bool sharp)
{
	if (motionState == EMotionStates::JUMPING) {
		this->wallJumpInitial = false;
		ChangeMovement(EMotionStates::RISING);
		if (sharp) {
			AddVelocity(FVector::DownVector, FMath::Abs(this->velocity.Z / (GetWorld()->GetDeltaSeconds() * 1.5)));
		}
	}
}

void AHollowKnight::Jump()
{
	StartJump();
}

void AHollowKnight::InterruptJump()
{
	StopJump(true);
}

void AHollowKnight::StartDash()
{
	if (this->dashCounter < 1) {
		this->dashCounter++;
		this->dashDirection = this->lookDirection;
		ChangeMovement(EMotionStates::DASHING);

		UKismetSystemLibrary::K2_ClearTimer(this, TEXT("Ungrounded"));
	}
}

void AHollowKnight::Dashing(float deltaSeconds)
{
	if (motionState == EMotionStates::DASHING) {
		AddVelocity(this->dashDirection, this->dashAcceleration);
		this->dashTimer += deltaSeconds;
		if (this->dashTimer >= this->dashDuration) {
			this->dashTimer = 0;
			StopDash();
		}
	}
}

void AHollowKnight::StopDash()
{
	//if (motionState == EMotionStates::DASHING) {
	//	ChangeMovement(EMotionStates::FALLING);
	//}
	ChangeMovement(EMotionStates::FALLING);
}


void AHollowKnight::Ungrounded()
{
	if (!IsInAir()) {
		ChangeMovement(EMotionStates::FALLING);
	}
}

bool AHollowKnight::IsInAir()
{
	return (motionState == EMotionStates::JUMPING || motionState == EMotionStates::FALLING || motionState == EMotionStates::RISING);
}

void AHollowKnight::Grounded()
{
	ChangeMovement(EMotionStates::GROUNDED);
	this->dashCounter = 0;
	this->doubleJump = true;
	UKismetSystemLibrary::K2_SetTimer(this, TEXT("Ungrounded"), .000001f, false); // timer resets each frame until not on ground
}

void AHollowKnight::OnWall(FVector hitNormal)
{
	this->dashCounter = 0;
	ChangeMovement(EMotionStates::WALLSLIDE);
	this->wallNormal = hitNormal;
	this->doubleJump = true;
	UKismetSystemLibrary::K2_SetTimer(this, TEXT("Ungrounded"), .000001f, false); // timer resets each frame until not on wall
}

void AHollowKnight::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{

	//Check if dot is sqrt(2)/2 (45 degrees)

	if (FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector) >= 0.71f) {
		Grounded();
	}
	else if(FMath::Abs(FVector::DotProduct(Hit.ImpactNormal, FVector::RightVector)) >= 0.71f) {
		OnWall(Hit.ImpactNormal);
	}
}

