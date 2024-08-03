// Fill out your copyright notice in the Description page of Project Settings.


#include "LabBuildingBase.h"

#include "GameGs.h"
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

bool ALabBuildingBase::ResearchRecipe(const int32 RecipeId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (GameState->Recipes.Num() <= RecipeId) return false;
	if (!InputItems(GameState->Recipes[RecipeId].Inputs)) return false;
	CurrentRecipe = RecipeId;
	MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, CurrentRecipe, this);
	OnRep_CurrentRecipe();
	const uint16 SpeedMultiplierTier = FMath::Min(Tier, TierSpeedMultipliers.Num() - 1);
	EndTimestamp = GameState->Recipes[RecipeId].ResearchBaseTime * TierSpeedMultipliers[SpeedMultiplierTier] + GetWorld()
		->GetTimeSeconds();
	MARK_PROPERTY_DIRTY_FROM_NAME(ALabBuildingBase, EndTimestamp, this);
	OnRep_EndTimestamp();
	return true;
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

void ALabBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetOwner()->HasAuthority()) return;

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
