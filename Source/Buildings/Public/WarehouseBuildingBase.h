// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "WarehouseBuildingBase.generated.h"

class UInventoryComponent;

UCLASS(Abstract)
class BUILDINGS_API AWarehouseBuildingBase : public ABuilding
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWarehouseBuildingBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnChangeTier() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetFilter(const TSubclassOf<UItem> Item);

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UInventoryComponent* InventoryComponent;

	//NOTE: This breaks if downgraded, another impl has to be written if that's the case.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<int32> CapacityIncreasePerTier;

	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnRep_Filter)
	TSubclassOf<UItem> Filter;

	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_Filter();
};
