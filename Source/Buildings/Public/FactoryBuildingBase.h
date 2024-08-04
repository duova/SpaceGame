// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBuildingBase.h"
#include "FactoryBuildingBase.generated.h"

struct FRecipe;
class AGameGs;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateFactory);

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

UCLASS(Abstract)
class BUILDINGS_API AFactoryBuildingBase : public AItemBuildingBase
{
	GENERATED_BODY()

public:
	AFactoryBuildingBase();

	//Sets the recipe, automatically sets count to 0, and refunds the most recent inputs.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ChangeRecipe(const int32 RecipeId);

	//Sets the number of the current recipe to be made. The current item being made is not included.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool SetCount(const int32 Count);

	//Modifies the number of the current recipe to be made by Count. The current item being made is not included.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ChangeCount(const int32 Count);

	UFUNCTION(BlueprintPure)
	float GetRemainingTime();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	AGameGs* GameState = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void RunRecipe(const FRecipe& Recipe);
	
	void CheckAndStart(const FRecipe& Recipe);

	UFUNCTION()
	void OnRep_Progress() const;

	UFUNCTION()
	void OnRep_CurrentRecipe() const;

	UFUNCTION()
	void OnRep_RemainingCount() const;

	UFUNCTION()
	void OnRep_Status() const;
	

public:
	virtual void Tick(float DeltaTime) override;

	//Decimal 0-1.
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_Progress)
	float Progress;

	//Recipe index with -1 being no recipe.
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_CurrentRecipe)
	int32 CurrentRecipe = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_RemainingCount)
	int32 RemainingCount;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_Status)
	EFactoryStatus Status;

	//Base recipe time is divided by this amount.
	//E.g. 6 sec at 2x will be 3 sec.
	//Indexed by tier. Keep in mind the factory ticks at a max of 5/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	TArray<float> TierSpeedMultipliers;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateFactory OnUpdateProgress;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateFactory OnUpdateCurrentRecipe;
	
	UPROPERTY(BlueprintAssignable)
	FOnUpdateFactory OnUpdateRemainingCount;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateFactory OnUpdateStatus;
};
