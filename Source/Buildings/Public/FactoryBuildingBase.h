// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "ItemBuildingBase.h"
#include "RecipeCatalog.h"
#include "FactoryBuildingBase.generated.h"

class UGameGi;

//Previous states are assumed to be active, e.g. if factory is lacking output it can be assumed that the recipe is set.
UENUM(BlueprintType)
enum class EFactoryStatus : uint8
{
	IdleNoRecipe,
	IdleNoCount,
	IdleNoInput,
	IdleNoOutput,
	Running
};

UENUM(BlueprintType)
enum class EBuildingRecipeValidationResult : uint8
{
	Success,
	Null,
	RecipeHasNoOutputs,
	NotABuildingRecipe,
	BuildingClassMismatch,
	InvalidBuildingTier,
	NotAnUpgrade,
	UpgradedByOtherFactory,
	InvalidRecipeId
};

UCLASS(Abstract)
class BUILDINGS_API AFactoryBuildingBase : public AItemBuildingBase
{
	GENERATED_BODY()

public:
	AFactoryBuildingBase();

	EBuildingRecipeValidationResult TargetBuildingValid(const ABuilding* TargetBuilding, const FRecipe& Recipe) const;

	//Check for items, but also if the target building is the same class as or subclass of the recipe building, as well
	//as if the tier is valid.
	UFUNCTION(BlueprintPure)
	bool CanPerformRecipe(const int32 RecipeId, const ABuilding* TargetBuilding = nullptr);

	//Sets the recipe, automatically sets count to 0, refunds the most recent inputs, and unlocks the previously upgraded building.
	//Set the target building if the recipe changes the tier of a building.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	EBuildingRecipeValidationResult ChangeRecipe(const int32 RecipeId, ABuilding* TargetBuilding = nullptr);

	//Sets the number of the current recipe to be made. The current item being made is not included.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetCount(const int32 Count);

	//Modifies the number of the current recipe to be made. The current item being made is not included.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ChangeCount(const int32 Count);

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	UGameGi* GameInstance = nullptr;

	bool TryLockBuildingForUpgrade(ABuilding* Building);

	bool UnlockBuildingPostUpgrade();

	UPROPERTY()
	ABuilding* LockedBuilding = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;

	//Decimal 0-1.
	UPROPERTY(BlueprintReadOnly, Replicated)
	float Progress;

	//Recipe index with -1 being no recipe.
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 CurrentRecipe = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 RemainingCount;

	UPROPERTY(BlueprintReadOnly, Replicated)
	EFactoryStatus Status;

	//Base recipe time is divided by this amount.
	//E.g. 6 sec at 2x will be 3 sec.
	//Indexed by tier. Keep in mind the factory ticks at a max of 5/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<float> TierSpeedMultipliers;
};
