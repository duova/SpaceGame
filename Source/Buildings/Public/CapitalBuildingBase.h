// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "CapitalBuildingBase.generated.h"

class UInventoryComponent;

USTRUCT()
struct FBuildSlotGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UBuildSlotComponent*> BuildSlots;
};

UCLASS(Abstract)
class BUILDINGS_API ACapitalBuildingBase : public ABuilding
{
	GENERATED_BODY()

public:
	ACapitalBuildingBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<FBuildSlotGroup> TieredBuildSlots;

	bool EnableTier(int32 InTier);
	
public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	TArray<UInventoryComponent*> LinkedInventories;
};
