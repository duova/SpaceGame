// Fill out your copyright notice in the Description page of Project Settings.


#include "WarehouseBuildingBase.h"

#include "InventoryComponent.h"


AWarehouseBuildingBase::AWarehouseBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
}

void AWarehouseBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWarehouseBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

