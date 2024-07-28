// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "OutputBuildingBase.h"
#include "FactoryBuildingBase.generated.h"

class UGameGi;

UCLASS(Abstract)
class BUILDINGS_API AFactoryBuildingBase : public AOutputBuildingBase
{
	GENERATED_BODY()

public:
	AFactoryBuildingBase();

	UFUNCTION(BlueprintPure)
	bool CanPerformRecipe(const int32 RecipeId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RequestRecipe(const int32 RecipeId, UBuildSlotComponent* TargetBuildingSlot = nullptr);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UGameGi* GameInstance = nullptr;

public:
	virtual void Tick(float DeltaTime) override;
};
