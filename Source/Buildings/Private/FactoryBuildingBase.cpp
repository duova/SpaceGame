// Fill out your copyright notice in the Description page of Project Settings.


#include "FactoryBuildingBase.h"

#include "GameGi.h"


AFactoryBuildingBase::AFactoryBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFactoryBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = Cast<UGameGi>(GetGameInstance());
}

void AFactoryBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

