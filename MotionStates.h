// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType, Category = "MotionState Enums")
namespace EMotionStates
{
	enum Type
	{
		GROUNDED = 0 UMETA(DisplayName = "GROUNDED"),
		JUMPING = 1 UMETA(DisplayName = "JUMPING"),
		RISING = 2 UMETA(DisplayName = "RISING"),
		FALLING = 3 UMETA(DisplayName = "FALLING"),
		DASHING = 4 UMETA(DisplayName = "DASHING"),
		WALLSLIDE = 5 UMETA(DisplayName = "WALLSLIDE")
	};
}