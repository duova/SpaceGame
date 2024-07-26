// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"

#include "Net/UnrealNetwork.h"

UItem::UItem(): Count(0), MaxStackSize(0)
{
}

bool UItem::IsSupportedForNetworking() const
{
	return true;
}

void UItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UItem, Count, RepParams);
}

int32 UItem::GetMaxStackSize() const
{
	return MaxStackSize;
}
