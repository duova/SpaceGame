// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePawn.h"

#include "GameAsc.h"
#include "GamePlayerState.h"
#include "InventoryComponent.h"

AGamePawn::AGamePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	Asc = CreateDefaultSubobject<UGameAsc>("AbilitySystemComponent");
	bReplicates = true;
	APawn::SetReplicateMovement(true);
}

void AGamePawn::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	}
}

void AGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGamePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UAbilitySystemComponent* AGamePawn::GetAbilitySystemComponent() const
{
	return Asc;
}

UInventoryComponent* AGamePawn::GetInventoryComponent() const
{
	return InventoryComp;
}

void AGamePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
}

