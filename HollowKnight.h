// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MotionStates.h"
#include "HollowKnight.generated.h"

UCLASS()
class PLASMOID_API AHollowKnight : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AHollowKnight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
		class USphereComponent* capsule;

	UPROPERTY(EditAnywhere)
		class UFloatingPawnMovement* pawnMovement;

	UPROPERTY(EditAnywhere)
		class UCameraComponent* camera;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//Dash
	float dashTimer;
	float dashDuration;
	float dashAcceleration;
	float dashMaxSpeed;
	FVector dashDirection;
	int dashCounter;
	void StartDash();
	void Dashing(float deltaSeconds);
	void StopDash();

	//Jump
	float jumpSpeedCap;
	float jumpAcceleration;
	float jumpTimer;
	float jumpMaxDuration;
	bool doubleJump;
	void StartJump();
	void Jumping(float deltaSeconds);
	void StopJump(bool sharp);
	void Jump();
	void InterruptJump();
	bool IsInAir();
	void Grounded();
	UFUNCTION()
		void Ungrounded();

	FVector lookDirection;

	//Movement
	FVector addedVelocity;
	FVector velocity;
	float maxWalkSpeed;
	EMotionStates::Type motionState;
	void AddVelocity(FVector direction, float scale);
	void ResolveMovement(float deltaSeconds);
	bool ChangeMovement(EMotionStates::Type newMotionState);
	void MoveHorizontal(float amount);

	//Wall Movement
	float wallFallSpeed;
	FVector wallNormal;
	bool landedOnWall;
	bool touchingWall;
	bool wallJumpInitial;
	FVector wallDirection;
	void OnWall(FVector hitNormal);
	void CheckWallCollision();
	UFUNCTION()
		void CompleteWallLanding();

	//Gravity
	float gravityAcceleration;
	float risingGravityAcceleration;
	float maxFallSpeed;
	void AddGravity();

	UFUNCTION()
		void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
};
