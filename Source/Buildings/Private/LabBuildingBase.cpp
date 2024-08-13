// Fill out your copyright notice in the Description page of Project Settings.


#include "LabBuildingBase.h"

#include "GameGs.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


ALabBuildingBase::ALabBuildingBase(): EndTimestamp(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5;
}

void ALabBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	GameState = Cast<AGameGs>(GetWorld()->GetGameState());
}

void ALabBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ALabBuildingBase, EndTimestamp, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ALabBuildingBase, CurrentRecipe, RepParams);
}

bool ALabBuildingBase::ResearchRecipe(const int32 RecipeId, UInventoryComponent* InvComp)
{
	if (!HasAuthority()) return false;
	if (CurrentRecipe != INDEX_NONE) return false;
	if (GameState->Recipes.Num() <= RecipeId) return false;
	if (!InvComp->RemoveItemsBatched(GameState->Recipes[RecipeId].Inputs)) return false;
	CurrentRecipe = RecipeId;
	MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, CurrentRecipe, this);
	OnRep_CurrentRecipe();
	const uint16 SpeedMultiplierTier = FMath::Min(Tier, TierSpeedMultipliers.Num() - 1);
	EndTimestamp = (GameState->Recipes[RecipeId].ResearchBaseTime / FMath::Max(0.1, TierSpeedMultipliers[SpeedMultiplierTier])) + GetWorld()
		->GetTimeSeconds();
	MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, EndTimestamp, this);
	OnRep_EndTimestamp();
	return true;
}

float ALabBuildingBase::GetRemainingTime() const
{
	return FMath::Max(0, EndTimestamp - GameState->GetServerWorldTimeSeconds());
}

float ALabBuildingBase::GetProgress()
{
	if (CurrentRecipe == INDEX_NONE) return 0;
	const uint16 SpeedMultiplierTier = FMath::Min(Tier, TierSpeedMultipliers.Num() - 1);
	return 1 - GetRemainingTime() / (GameState->Recipes[CurrentRecipe].ResearchBaseTime / TierSpeedMultipliers[SpeedMultiplierTier]);
}

void ALabBuildingBase::OnRep_EndTimestamp() const
{
	if (OnUpdateEndTimestamp.IsBound())
	{
		OnUpdateEndTimestamp.Broadcast();
	}
}

void ALabBuildingBase::OnRep_CurrentRecipe() const
{
	if (OnUpdateCurrentRecipe.IsBound())
	{
		OnUpdateCurrentRecipe.Broadcast();
	}
}

void ALabBuildingBase::OnChangeTier()
{
	Super::OnChangeTier();

	if (!HasAuthority()) return;
	if (CurrentRecipe == INDEX_NONE) return;
	const uint16 LastTier = Tier - 1;
	if (LastTier > 0) return;
	const float OriginalResearchTime = GameState->Recipes[CurrentRecipe].ResearchBaseTime / FMath::Max(0.1, TierSpeedMultipliers[LastTier]);
	const float Progress = 1 - (EndTimestamp - GetWorld()->GetTimeSeconds()) / OriginalResearchTime;
	const float NewResearchTime = GameState->Recipes[CurrentRecipe].ResearchBaseTime / FMath::Max(0.1, TierSpeedMultipliers[Tier]);
	const float NewRemainingTime = NewResearchTime * (1 - Progress);
	EndTimestamp = GetWorld()->GetTimeSeconds() + NewRemainingTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, EndTimestamp, this);
	OnRep_EndTimestamp();
}

void ALabBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!HasAuthority()) return;

	if (GetWorld()->GetTimeSeconds() > EndTimestamp && CurrentRecipe != INDEX_NONE)
	{
		EndTimestamp = 0;
		MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, EndTimestamp, this);
		OnRep_EndTimestamp();
		GameState->Recipes[CurrentRecipe].bUnlocked = true;
		GameState->MarkRecipesDirty();
		CurrentRecipe = INDEX_NONE;
		MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, CurrentRecipe, this);
		OnRep_CurrentRecipe();
	}
}
