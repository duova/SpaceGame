﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UItem::UItem(): Icon(nullptr), OwningInvComp(nullptr), OwningInvIndex(0)
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
	DOREPLIFETIME_WITH_PARAMS_FAST(UItem, OrderedAbilityHandles, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UItem, OwningInvComp, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UItem, OwningInvIdentifier, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UItem, OwningInvIndex, RepParams);
}

int32 UItem::GetMaxStackSize() const
{
	return MaxStackSize;
}

void UItem::OnRep_Count() const
{
	OwningInvComp->InternalOnItemUpdate();
}
