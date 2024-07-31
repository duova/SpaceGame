// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBuildingBase.h"
#include "MineBuildingBase.generated.h"

UCLASS(Abstract)
class BUILDINGS_API AMineBuildingBase : public AItemBuildingBase
{
	GENERATED_BODY()

public:
	AMineBuildingBase();

protected:
	virtual void BeginPlay() override;

	void EvaluateRate();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Base resource rate is multiplied by this.
	//Indexed by tier.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	TArray<int32> TierSpeedMultipliers;

	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnRep_RatePerMinute)
	int32 RatePerMinute;

	bool bRateEvaluated = false;

	float UnrealizedResources;

	virtual void OnChangeTier() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_RatePerMinute();
};
