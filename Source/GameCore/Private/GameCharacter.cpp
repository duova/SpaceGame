﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "GameCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameAsc.h"
#include "GamePlayerState.h"

AGameCharacter::AGameCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	bReplicates = true;
	ACharacter::SetReplicateMovement(true);
}

void AGameCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UAbilitySystemComponent* AGameCharacter::GetAbilitySystemComponent() const
{
	return Asc;
}

UInventoryComponent* AGameCharacter::GetInventoryComponent() const
{
	return InventoryComp;
}

void AGameCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AGamePlayerState* Ps = GetPlayerState<AGamePlayerState>())
	{
		//Set the Asc on the Server. Clients do this in OnRep_PlayerState().
		Asc = Cast<UGameAsc>(Ps->GetAbilitySystemComponent());
		Ps->GetAbilitySystemComponent()->InitAbilityActorInfo(Ps, this);
	}
}

void AGameCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AGamePlayerState* Ps = GetPlayerState<AGamePlayerState>())
	{
		// Set the Asc for clients. Server does this in PossessedBy.
		Asc = Cast<UGameAsc>(Ps->GetAbilitySystemComponent());
		Asc->InitAbilityActorInfo(Ps, this);
	}
}

