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

	UFUNCTION(BlueprintPure)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventoryComponent() const;

	virtual void PossessedBy(AController* NewController) override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGameAsc* Asc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UInventoryComponent* InventoryComp;
};
