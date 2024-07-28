// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "InventoryComponent.h"
#include "GameFramework/Character.h"
#include "GameCharacter.generated.h"

class UGameAsc;
/**
 * Character for this game with GAS.
 */
UCLASS()
class GAMECORE_API AGameCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGameCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
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
