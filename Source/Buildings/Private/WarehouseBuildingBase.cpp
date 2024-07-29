// Fill out your copyright notice in the Description page of Project Settings.


#include "WarehouseBuildingBase.h"

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"


AWarehouseBuildingBase::AWarehouseBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
}

void AWarehouseBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWarehouseBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AWarehouseBuildingBase, Filter, RepParams);
}

void AWarehouseBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWarehouseBuildingBase::OnChangeTier()
{
	Super::OnChangeTier();

	if (InventoryComponent->Inventories.Num() < 1) return;
	if (CapacityIncreasePerTier.Num() <= Tier) return;
	InventoryComponent->IncreaseCapacity(InventoryComponent->Inventories[0].InventoryIdentifier, CapacityIncreasePerTier[Tier]);
}

void AWarehouseBuildingBase::SetFilter(const TSubclassOf<UItem> Item)
{
	Filter = Item;
	MARK_PROPERTY_DIRTY_FROM_NAME(AWarehouseBuildingBase, Filter, this);
}

