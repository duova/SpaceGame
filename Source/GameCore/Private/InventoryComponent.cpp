// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "GameCore.h"
#include "Item.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

bool FInventory::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << InventoryIdentifier;
	Ar << Capacity;
	Ar << EmptyItemClass;

	uint16 Size = Items.Num();
	Ar << Size;
	if (!Ar.IsSaving())
	{
		Items.Empty();
		Items.AddDefaulted(Size);
	}
	for (UItem*& Item : Items)
	{
		UObject* Ref = Item;
		Map->SerializeObject(Ar, Item->GetClass(), Ref);
		Item = Cast<UItem>(Ref);
	}

	return bOutSuccess;
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		//Remove duplicate inventory identifiers if necessary.
		for (int16 i = Inventories.Num() - 1; i >= 0; i--)
		{
			bool bHasDuplicate = false;
			for (int16 j = i - 1; j >= 0; j--)
			{
				if (Inventories[j].InventoryIdentifier == Inventories[i].InventoryIdentifier)
				{
					bHasDuplicate = true;
					break;
				}
			}
			if (bHasDuplicate)
			{
				UE_LOG(LogGameCore, Warning, TEXT("Duplicate inventory identifier %s. Removing."),
				       *Inventories[i].InventoryIdentifier.ToString());
				Inventories.RemoveAt(i, 1, false);
			}
		}
		Inventories.Shrink();

		//Fill with empty items.
		for (FInventory& Inventory : Inventories)
		{
			for (uint16 i = 0; i < Inventory.Capacity; i++)
			{
				const uint16 Index = Inventory.Items.Emplace(NewObject<UItem>(this, Inventory.EmptyItemClass));
				AddReplicatedSubObject(Inventory.Items[Index]);
			}
		}

		MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
	}
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, Inventories, RepParams);
}

UItem* UInventoryComponent::AddToInventory(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass,
                                           const int32 Count, const int32 Index)
{
	if (Count < 1) return nullptr;
	if (Count >= ItemClass.GetDefaultObject()->GetMaxStackSize()) return nullptr;
	for (FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == InventoryIdentifier)
		{
			if (Index >= Inventory.Capacity) return nullptr;
			if (Index >= Inventory.Items.Num()) return nullptr;
			if (Inventory.Items[Index]->GetClass() != Inventory.EmptyItemClass) return nullptr;
			RemoveReplicatedSubObject(Inventory.Items[Index]);
			Inventory.Items[Index] = NewObject<UItem>(this, ItemClass);
			AddReplicatedSubObject(Inventory.Items[Index]);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
			Inventory.Items[Index]->Count = Count;
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory.Items[Index]);
			return Inventory.Items[Index];
		}
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		InternalOnItemUpdate();
		//Client side called OnRep.
	}
	
	return nullptr;
}

bool UInventoryComponent::RemoveFromInventory(const FGameplayTag InventoryIdentifier, const int32 Index)
{
	for (FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == InventoryIdentifier)
		{
			if (Index >= Inventory.Capacity) return false;
			if (Index >= Inventory.Items.Num()) return false;
			if (Inventory.Items[Index]->GetClass() == Inventory.EmptyItemClass) return false;
			RemoveReplicatedSubObject(Inventory.Items[Index]);
			Inventory.Items[Index] = NewObject<UItem>(this, Inventory.EmptyItemClass);
			AddReplicatedSubObject(Inventory.Items[Index]);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
			return true;
		}
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		InternalOnItemUpdate();
		//Client side called OnRep.
	}
	
	return false;
}

void UInventoryComponent::InternalMoveOrSwapItems(const UInventoryComponent* OtherInventoryComponent,
                                                  const int32 OtherItemIndex, const int32 LocalItemIndex, UItem* Other,
                                                  FInventory* OtherInventory, UItem* Local, FInventory* LocalInventory)
{
	if (Other)
	{
		RemoveReplicatedSubObject(LocalInventory->Items[LocalItemIndex]);
		LocalInventory->Items[LocalItemIndex] = Other;
	}
	else
	{
		LocalInventory->Items[LocalItemIndex] = NewObject<UItem>(this, LocalInventory->EmptyItemClass);
	}

	if (Local)
	{
		RemoveReplicatedSubObject(OtherInventory->Items[OtherItemIndex]);
		OtherInventory->Items[OtherItemIndex] = Local;
	}
	else
	{
		OtherInventory->Items[OtherItemIndex] = NewObject<UItem>(this, OtherInventory->EmptyItemClass);
	}

	AddReplicatedSubObject(LocalInventory->Items[LocalItemIndex]);
	AddReplicatedSubObject(LocalInventory->Items[LocalItemIndex]);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, OtherInventoryComponent);
}

void UInventoryComponent::InternalOnItemUpdate() const
{
	if (OnItemUpdate.IsBound()) OnItemUpdate.Broadcast();
}

bool UInventoryComponent::MoveOrSwapItem(UInventoryComponent* OtherInventoryComponent,
                                         const FGameplayTag OtherInventoryIdentifier, const int32 OtherItemIndex,
                                         const FGameplayTag LocalInventoryIdentifier,
                                         const int32 LocalItemIndex, const bool bStackIfPossible)
{
	UItem* Other = nullptr;
	FInventory* OtherInventory = nullptr;
	UItem* Local = nullptr;
	FInventory* LocalInventory = nullptr;

	for (FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == LocalInventoryIdentifier)
		{
			if (LocalItemIndex >= Inventory.Capacity) return false;
			if (LocalItemIndex >= Inventory.Items.Num()) return false;
			Local = Inventory.Items[LocalItemIndex];
			LocalInventory = &Inventory;
			break;
		}
	}
	if (!Local) return false;
	//Use nullptr to indicate empty item from this point as opposed to inventory not found.
	if (LocalInventory->EmptyItemClass == Local->GetClass()) Local = nullptr;

	if (!OtherInventoryComponent) return false;
	for (FInventory& Inventory : OtherInventoryComponent->Inventories)
	{
		if (Inventory.InventoryIdentifier == OtherInventoryIdentifier)
		{
			if (OtherItemIndex >= Inventory.Capacity) return false;
			if (OtherItemIndex >= Inventory.Items.Num()) return false;
			Other = Inventory.Items[OtherItemIndex];
			OtherInventory = &Inventory;
			break;
		}
	}
	if (!Other) return false;
	//Use nullptr to indicate empty item from this point as opposed to inventory not found.
	if (OtherInventory->EmptyItemClass == Other->GetClass()) Other = nullptr;

	if (bStackIfPossible && Other && Local && Other->GetClass() == Local->GetClass())
	{
		Other->Count += Local->Count;
		MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Other);
		RemoveFromInventory(LocalInventoryIdentifier, LocalItemIndex);
	}

	InternalMoveOrSwapItems(OtherInventoryComponent, OtherItemIndex, LocalItemIndex, Other, OtherInventory, Local,
	                        LocalInventory);

	if (GetOwnerRole() == ROLE_Authority)
	{
		InternalOnItemUpdate();
		//Client side called OnRep.
	}
	
	return true;
}

TArray<UItem*> UInventoryComponent::GetItems(const FGameplayTag InventoryIdentifier) const
{
	for (const FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == InventoryIdentifier)
		{
			return Inventory.Items;
		}
	}
	return TArray<UItem*>();
}
