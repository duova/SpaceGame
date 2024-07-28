// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlayerState.h"

#include "AbilitySystemComponent.h"
#include "GameAsc.h"

AGamePlayerState::AGamePlayerState()
{
	Asc = CreateDefaultSubobject<UGameAsc>(TEXT("AbilitySystemComponent"));
	Asc->SetIsReplicated(true);
}

UAbilitySystemComponent* AGamePlayerState::GetAbilitySystemComponent() const
{
	return Asc;
}
