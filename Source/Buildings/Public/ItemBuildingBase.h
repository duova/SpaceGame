// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "ItemBuildingBase.generated.h"

struct FItemDescriptor;
class UInventoryComponent;
class UItem;

/**
 * Helper functions to check, grab, and dump items to warehouses.
 * Buffers output if there are no inventories to throw items into.
 */
UCLASS(Abstract)
class BUILDINGS_API AItemBuildingBase : public ABuilding
{
	GENERATED_BODY()

public:
	AItemBuildingBase();

	bool HasItems(const TArray<FItemDescriptor>& Items);

	//Atomic action, will not use any items if not all items are available.
	bool InputItems(const TArray<FItemDescriptor>& Items, const bool bAssumeEnoughItems = false);
	
	bool OutputItems(const TArray<FItemDescriptor>& Items);

	UFUNCTION(BlueprintPure)
	bool IsOutputLocked() const;

protected:
	virtual void BeginPlay() override;
	
	void RefreshNearbyInventories();

	bool TryFlush();

	virtual void Tick(float DeltaSeconds) override;

private:
	bool bOutputLocked;

	UPROPERTY()
	TArray<UInventoryComponent*> NearbyInventories;

	UPROPERTY()
	UInventoryComponent* Buffer;
	
	bool InternalHasItems(const TMap<const TSubclassOf<UItem>, int32>& JoinedItemRequirements);
};
