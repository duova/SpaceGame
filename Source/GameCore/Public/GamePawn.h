// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GamePawn.generated.h"

class UGameAsc;
class UInventoryComponent;

/**
 * Pawn for this game with GAS.
 */
UCLASS()
class GAMECORE_API AGamePawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGamePawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UInventoryComponent* GetInventoryComponent() const;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;
	
protected:
	UPROPERTY()
	UGameAsc* Asc;

	UPROPERTY()
	UInventoryComponent* InventoryComp;
};
