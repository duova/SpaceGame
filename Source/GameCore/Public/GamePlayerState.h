// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GamePlayerState.generated.h"

class UGameAsc;
/**
 * PlayerState for this game with GAS.
 */
UCLASS()
class GAMECORE_API AGamePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGamePlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	UPROPERTY()
	UGameAsc* Asc;
};
