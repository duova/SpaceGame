// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePawn.h"

#include "GameAsc.h"
#include "GamePlayerState.h"
#include "InventoryComponent.h"

AGamePawn::AGamePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	SetReplicates(true);
	APawn::SetReplicateMovement(true);
}

void AGamePawn::BeginPlay()
{
	Super::BeginPlay();
	
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

	if (AGamePlayerState* Ps = GetPlayerState<AGamePlayerState>())
	{
		//Set the Asc on the Server. Clients do this in OnRep_PlayerState().
		Asc = Cast<UGameAsc>(Ps->GetAbilitySystemComponent());
		Ps->GetAbilitySystemComponent()->InitAbilityActorInfo(Ps, this);
	}
}

void AGamePawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AGamePlayerState* Ps = GetPlayerState<AGamePlayerState>())
	{
		// Set the Asc for clients. Server does this in PossessedBy.
		Asc = Cast<UGameAsc>(Ps->GetAbilitySystemComponent());
		Asc->InitAbilityActorInfo(Ps, this);
	}
}

