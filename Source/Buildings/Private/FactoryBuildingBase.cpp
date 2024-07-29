// Fill out your copyright notice in the Description page of Project Settings.


#include "FactoryBuildingBase.h"

#include "GameGi.h"
#include "RecipeCatalog.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


AFactoryBuildingBase::AFactoryBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.TickInterval = 0.2;
}

EBuildingRecipeValidationResult AFactoryBuildingBase::TargetBuildingValid(
	const ABuilding* TargetBuilding, const FRecipe& Recipe) const
{
	if (!TargetBuilding) return EBuildingRecipeValidationResult::Null;
	if (Recipe.Result.Num() <= 0) return EBuildingRecipeValidationResult::RecipeHasNoOutputs;
	const UBuildingRecipeResult* BuildingResult = Cast<UBuildingRecipeResult>(Recipe.Result[0]);
	if (!BuildingResult) return EBuildingRecipeValidationResult::NotABuildingRecipe;
	if (!TargetBuilding->IsA(BuildingResult->GetClass())) return EBuildingRecipeValidationResult::BuildingClassMismatch;
	if (TargetBuilding->Tiers.Num() <= BuildingResult->BuildingTier) return
		EBuildingRecipeValidationResult::InvalidBuildingTier;
	if (BuildingResult->BuildingTier <= TargetBuilding->GetTier()) return EBuildingRecipeValidationResult::NotAnUpgrade;
	if (TargetBuilding->UpgradeLockedBy && TargetBuilding->UpgradeLockedBy != this) return
		EBuildingRecipeValidationResult::UpgradedByOtherFactory;
	return EBuildingRecipeValidationResult::Success;
}

bool AFactoryBuildingBase::CanPerformRecipe(const int32 RecipeId, const ABuilding* TargetBuilding)
{
	if (GameInstance->RecipeCatalog->Recipes.Num() <= RecipeId) return false;
	if (GameInstance->RecipeUnlocks.Num() <= RecipeId) return false;
	if (!GameInstance->RecipeUnlocks[RecipeId]) return false;
	const FRecipe& Recipe = GameInstance->RecipeCatalog[RecipeId].Recipes[RecipeId];
	if (TargetBuildingValid(TargetBuilding, Recipe) != EBuildingRecipeValidationResult::Success) return false;
	return HasItems(Recipe.Inputs);
}

EBuildingRecipeValidationResult AFactoryBuildingBase::ChangeRecipe(const int32 RecipeId, ABuilding* TargetBuilding)
{
	if (GameInstance->RecipeCatalog->Recipes.Num() <= RecipeId) return EBuildingRecipeValidationResult::InvalidRecipeId;
	const FRecipe& Recipe = GameInstance->RecipeCatalog[RecipeId].Recipes[RecipeId];
	if (TargetBuilding)
	{
		EBuildingRecipeValidationResult Validation = TargetBuildingValid(TargetBuilding, Recipe);
		if (Validation != EBuildingRecipeValidationResult::Success) return Validation;
	}

	if (CurrentRecipe != INDEX_NONE && Status == EFactoryStatus::Running)
	{
		OutputItems(GameInstance->RecipeCatalog->Recipes[CurrentRecipe].Inputs);
	}

	UnlockBuildingPostUpgrade();
	Progress = 0;
	RemainingCount = 0;
	CurrentRecipe = RecipeId;
	Status = EFactoryStatus::IdleNoCount;
	TryLockBuildingForUpgrade(TargetBuilding);

	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Progress, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, CurrentRecipe, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, RemainingCount, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Status, this);

	return EBuildingRecipeValidationResult::Success;
}

void AFactoryBuildingBase::SetCount(const int32 Count)
{
}

void AFactoryBuildingBase::ChangeCount(const int32 Count)
{
}

void AFactoryBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = Cast<UGameGi>(GetGameInstance());
}

void AFactoryBuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnlockBuildingPostUpgrade();
}

bool AFactoryBuildingBase::TryLockBuildingForUpgrade(ABuilding* Building)
{
	if (Building->UpgradeLockedBy) return false;
	Building->UpgradeLockedBy = this;
	LockedBuilding = Building;
	return true;
}

bool AFactoryBuildingBase::UnlockBuildingPostUpgrade()
{
	if (!LockedBuilding) return false;
	LockedBuilding->UpgradeLockedBy = nullptr;
	LockedBuilding = nullptr;
	return true;
}

void AFactoryBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFactoryBuildingBase, Progress, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFactoryBuildingBase, CurrentRecipe, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFactoryBuildingBase, RemainingCount, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFactoryBuildingBase, Status, RepParams);
}

void AFactoryBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Ensure we only start a recipe if the output isn't locked so we can switch recipes and not fill up the buffer if
	//there are no outputs.
}
