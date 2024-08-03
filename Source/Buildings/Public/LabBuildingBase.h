// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBuildingBase.h"
#include "LabBuildingBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateLab);

class AGameGs;

UCLASS(Abstract)
class BUILDINGS_API ALabBuildingBase : public AItemBuildingBase
{
	GENERATED_BODY()

public:
	ALabBuildingBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	AGameGs* GameState = nullptr;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ResearchRecipe(const int32 RecipeId);

	UFUNCTION()
	void OnRep_EndTimestamp() const;

	UFUNCTION()
	void OnRep_CurrentRecipe() const;
	
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_EndTimestamp)
	double EndTimestamp;

	//Recipe index with -1 being no recipe.
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_CurrentRecipe)
	int32 CurrentRecipe = INDEX_NONE;

	//Base research time is divided by this amount.
	//E.g. 6 sec at 2x will be 3 sec.
	//Indexed by tier.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	TArray<float> TierSpeedMultipliers;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateLab OnUpdateEndTimestamp;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateLab OnUpdateCurrentRecipe;
};
