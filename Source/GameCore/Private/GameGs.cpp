﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "GameGs.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

bool FRecipe::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar.SerializeBits(&bUnlocked, 1);
	return bOutSuccess;
}

AGameGs::AGameGs()
{
	SetReplicates(true);
}

void AGameGs::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AGameGs, Recipes, RepParams);
}

void AGameGs::MarkRecipesDirty() const
{
	MARK_PROPERTY_DIRTY_FROM_NAME(AGameGs, Recipes, this);
}

void AGameGs::OnRep_Recipes() const
{
	if (!OnRecipeUnlocksUpdated.IsBound()) return;
	OnRecipeUnlocksUpdated.Broadcast();
}
