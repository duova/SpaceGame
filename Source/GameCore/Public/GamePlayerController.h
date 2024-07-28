// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayerController.generated.h"

class UItem;
class AGameCharacter;

UCLASS()
class UInputPayload : public UObject
{
	GENERATED_BODY()

public:
	FVector AimLocation;
};

/**
 * 
 */
UCLASS()
class GAMECORE_API AGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGamePlayerController();
	
	//Calculate aim location based on mouse or stick input.
	FVector CurrentAimLocation;

	UPROPERTY(EditAnywhere)
	TArray<UInputAction*> InventoryInputs;

protected:
	virtual void SetupInputComponent() override;
	
	void OnInputDown(const UInputAction* Input);

	void OnInputUp(const UInputAction* Input);

	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	UInputPayload* InputPayload;

	UPROPERTY()
	AGameCharacter* Char;
};
