// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "OutputBuildingBase.generated.h"

class UItemDescriptor;
class UInventoryComponent;
class UItem;

/**
 * Buffers output if there are no inventories to throw items into.
 */
UCLASS(Abstract)
class BUILDINGS_API AOutputBuildingBase : public ABuilding
{
	GENERATED_BODY()

public:
	AOutputBuildingBase();
	
	bool OutputItems(const TArray<UItemDescriptor>& Items);

	UFUNCTION(BlueprintPure)
	bool IsOutputLocked() const;

protected:
	virtual void BeginPlay() override;
	
	bool TryFlush();

	virtual void Tick(float DeltaSeconds) override;

private:
	bool bOutputLocked;

	UPROPERTY()
	TArray<UInventoryComponent*> OutputInventories;

	UPROPERTY()
	UInventoryComponent* Buffer;
};
