// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingNiagaraComponent.h"

#include "Building.h"
#include "GameCore.h"

UBuildingNiagaraComponent::UBuildingNiagaraComponent(): Building(nullptr)
{
}

void UBuildingNiagaraComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer) return;
	
	Building = Cast<ABuilding>(GetOwner());
	
	if (!Building)
	{
		UE_LOG(LogGameCore, Warning, TEXT("UBuildingNiagaraComponent cannot be placed on non-building actor."));
		return;
	}

	Building->InternalOnUpdateTier.AddUObject(this, &UBuildingNiagaraComponent::OnUpdateTier);
}

void UBuildingNiagaraComponent::OnUpdateTier()
{
	if (ActiveOnTiers.Contains(Building->GetTier()))
	{
		Activate(true);
	}
	else
	{
		Deactivate();
	}
}
