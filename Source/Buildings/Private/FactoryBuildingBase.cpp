// Fill out your copyright notice in the Description page of Project Settings.


#include "FactoryBuildingBase.h"

#include "GameGs.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


AFactoryBuildingBase::AFactoryBuildingBase(): Progress(0), RemainingCount(0), Status()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.TickInterval = 0.2;
}

bool AFactoryBuildingBase::ChangeRecipe(const int32 RecipeId)
{
	if (!HasAuthority()) return false;
	if (GameState->Recipes.Num() <= RecipeId) return false;
	if (!GameState->Recipes[RecipeId].bUnlocked) return false;

	if (CurrentRecipe != INDEX_NONE && Status == EFactoryStatus::Running)
	{
		OutputItems(GameState->Recipes[CurrentRecipe].Inputs);
	}

	Progress = 0;
	RemainingCount = 0;
	CurrentRecipe = RecipeId;
	Status = EFactoryStatus::IdleNoCount;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Progress, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, CurrentRecipe, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, RemainingCount, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Status, this);
	OnRep_Progress();
	OnRep_CurrentRecipe();
	OnRep_RemainingCount();
	OnRep_Status();

	return true;
}

bool AFactoryBuildingBase::SetCount(const int32 Count)
{
	if (!HasAuthority()) return false;
	RemainingCount = FMath::Max(0, Count);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, RemainingCount, this);
	OnRep_RemainingCount();
	return true;
}

bool AFactoryBuildingBase::ChangeCount(const int32 Count)
{
	if (!HasAuthority()) return false;
	RemainingCount = FMath::Max(0, Count + RemainingCount);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, RemainingCount, this);
	OnRep_RemainingCount();
	return true;
}

void AFactoryBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	GameState = Cast<AGameGs>(GetWorld()->GetGameState());
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

void AFactoryBuildingBase::RunRecipe()
{
	RemainingCount--;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, RemainingCount, this);
	OnRep_RemainingCount();
	Status = EFactoryStatus::Running;
}

void AFactoryBuildingBase::CheckAndStart(const FRecipe& Recipe)
{
	if (!HasItems(Recipe.Inputs))
	{
		Status = EFactoryStatus::IdleNoInput;
		MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Status, this);
		OnRep_Status();
	}
	else if (IsOutputLocked())
	{
		Status = EFactoryStatus::IdleNoOutput;
		MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Status, this);
		OnRep_Status();
	}
	else
	{
		RunRecipe();
	}
}

void AFactoryBuildingBase::OnRep_Progress() const
{
	if (OnUpdateProgress.IsBound())
	{
		OnUpdateProgress.Broadcast();
	}
}

void AFactoryBuildingBase::OnRep_CurrentRecipe() const
{
	if (OnUpdateCurrentRecipe.IsBound())
	{
		OnUpdateCurrentRecipe.Broadcast();
	}
}

void AFactoryBuildingBase::OnRep_RemainingCount() const
{
	if (OnUpdateRemainingCount.IsBound())
	{
		OnUpdateRemainingCount.Broadcast();
	}
}

void AFactoryBuildingBase::OnRep_Status() const
{
	if (OnUpdateStatus.IsBound())
	{
		OnUpdateStatus.Broadcast();
	}
}

void AFactoryBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetOwner()->HasAuthority()) return;
	
	if (CurrentRecipe < 0) return;
	if (CurrentRecipe >= GameState->Recipes.Num()) return;
	const FRecipe& Recipe = GameState->Recipes[CurrentRecipe];
	const uint16 SpeedMultiplierTier = FMath::Min(Tier, TierSpeedMultipliers.Num() - 1);

	if (Status == EFactoryStatus::Running)
	{
		Progress += DeltaTime * TierSpeedMultipliers[SpeedMultiplierTier] / Recipe.RecipeBaseTime;
		MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Progress, this);
		OnRep_Progress();
	}

	if (Progress > 1)
	{
		if (RemainingCount > 0)
		{
			Progress -= 1.0;
			MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Progress, this);
			OnRep_Progress();
			OutputItems(Recipe.Result);
			CheckAndStart(Recipe);
		}
		else
		{
			Status = EFactoryStatus::IdleNoCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Status, this);
			OnRep_Status();
			Progress = 0;
			MARK_PROPERTY_DIRTY_FROM_NAME(AFactoryBuildingBase, Progress, this);
			OnRep_Progress();
			OutputItems(Recipe.Result);
		}
	}

	if (RemainingCount > 0 && Status != EFactoryStatus::Running)
	{
		CheckAndStart(Recipe);
	}
}
