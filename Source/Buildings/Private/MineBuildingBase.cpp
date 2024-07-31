// Fill out your copyright notice in the Description page of Project Settings.


#include "MineBuildingBase.h"

#include "BuildSlotComponent.h"
#include "GameGs.h"
#include "Item.h"
#include "Net/UnrealNetwork.h"


AMineBuildingBase::AMineBuildingBase(): RatePerMinute(0), UnrealizedResources(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2;
}

void AMineBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMineBuildingBase::EvaluateRate()
{
	const uint16 SpeedMultiplierTier = FMath::Min(Tier, TierSpeedMultipliers.Num() - 1);
	RatePerMinute = TierSpeedMultipliers[SpeedMultiplierTier] * Slot->ResourceRatePerMinute;
	MARK_PROPERTY_DIRTY_FROM_NAME(AMineBuildingBase, RatePerMinute, this);
}

void AMineBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;

	if (!Slot) return;

	if (Slot->Resource.Get() == nullptr) return;

	if (!bRateEvaluated)
	{
		EvaluateRate();
		bRateEvaluated = true;
	}

	UnrealizedResources += DeltaTime * RatePerMinute / 60.0;
	const int32 ToRealize = FMath::FloorToInt(UnrealizedResources);
	UnrealizedResources -= ToRealize;
	TArray<FItemDescriptor> Out;
	Out.Emplace(Slot->Resource, ToRealize);
	OutputItems(Out);
}

void AMineBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AMineBuildingBase, RatePerMinute, RepParams);
}

void AMineBuildingBase::OnChangeTier()
{
	Super::OnChangeTier();

	bRateEvaluated = false;
}

