// Fill out your copyright notice in the Description page of Project Settings.


#include "CapitalBuildingBase.h"

#include "BuildSlotComponent.h"

ACapitalBuildingBase::ACapitalBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACapitalBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	if (!Slot) return;

	TieredBuildSlots.AddDefaulted(Tiers.Num());
	
	TArray<UBuildSlotComponent*> Slots;
	Slot->GetOwner()->GetComponents<UBuildSlotComponent>(Slots);

	for (UBuildSlotComponent* NearbySlot : Slots)
	{
		if (NearbySlot == Slot) continue;
		NearbySlot->Disable();
		if (NearbySlot->UnlockTier >= TieredBuildSlots.Num()) continue;
		TieredBuildSlots[NearbySlot->UnlockTier].BuildSlots.Add(NearbySlot);
	}

	EnableTier(0);
}

void ACapitalBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ACapitalBuildingBase::EnableTier(const int32 InTier)
{
	if (InTier >= TieredBuildSlots.Num()) return false;
	for (UBuildSlotComponent* NearbySlot : TieredBuildSlots[InTier].BuildSlots)
	{
		NearbySlot->Enable();
	}
	return true;
}

