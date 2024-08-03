// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBuildingBase.h"

#include "BuildSlotComponent.h"
#include "GameCharacter.h"
#include "GameGs.h"
#include "InventoryComponent.h"
#include "Item.h"
#include "WarehouseBuildingBase.h"

AItemBuildingBase::AItemBuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2;
	Buffer = CreateDefaultSubobject<UInventoryComponent>("Buffer");
	FInventory BufferInventory;
	BufferInventory.Capacity = 20;
	BufferInventory.InventoryIdentifier = FGameplayTag();
	BufferInventory.EmptyItemClass = UItem::StaticClass();
	Buffer->Inventories.Add(BufferInventory);
}

bool AItemBuildingBase::InternalHasItems(const TMap<const TSubclassOf<UItem>, int32>& JoinedItemRequirements)
{
	TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirementsCopy = JoinedItemRequirements;
	for (const UInventoryComponent* NearbyInvComp : NearbyInventories)
	{
		for (const UItem* Item : NearbyInvComp->GetItemsInAllInventories())
		{
			if (!JoinedItemRequirementsCopy.Contains(Item->GetClass())) continue;
			JoinedItemRequirementsCopy[Item->GetClass()] -= Item->Count;
		}
	}

	for (const TPair<const TSubclassOf<UItem>, int32>& Pair : JoinedItemRequirementsCopy)
	{
		if (Pair.Value > 0) return false;
	}

	return true;
}

bool AItemBuildingBase::HasItems(const TArray<FItemDescriptor>& Items)
{
	RefreshNearbyInventories();

	const TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements = UInventoryComponent::GetJoinedRequirements(Items);

	return InternalHasItems(JoinedItemRequirements);
}

bool AItemBuildingBase::InputItems(const TArray<FItemDescriptor>& Items, const bool bAssumeEnoughItems)
{
	RefreshNearbyInventories();

	TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements =  UInventoryComponent::GetJoinedRequirements(Items);
	
	if (!bAssumeEnoughItems && !InternalHasItems(JoinedItemRequirements)) return false;

	for (UInventoryComponent* NearbyInvComp : NearbyInventories)
	{
		for (FInventory& Inventory : NearbyInvComp->Inventories)
		{
			for (uint16 i = 0; i < Inventory.Items.Num(); i++)
			{
				UItem* Item = Inventory.Items[i];
				if (!JoinedItemRequirements.Contains(Item->GetClass())) continue;
				int32 TransferCount = FMath::Min(Item->Count, JoinedItemRequirements[Item->GetClass()]);
				NearbyInvComp->RemoveItem(Inventory.InventoryIdentifier, i, TransferCount);
				JoinedItemRequirements[Item->GetClass()] -= TransferCount;
			}
		}
	}

	return true;
}

bool AItemBuildingBase::OutputItems(const TArray<FItemDescriptor>& Items)
{
	if (IsOutputLocked()) return false;
	for (const FItemDescriptor& Item : Items)
	{
		Buffer->AddItemAnySlot(FGameplayTag(), Item.ItemClass, Item.ItemCount);
	}
	bOutputLocked = true;
	if (TryFlush())
	{
		bOutputLocked = false;
	}
	return true;
}

bool AItemBuildingBase::IsOutputLocked() const
{
	return bOutputLocked;
}

void AItemBuildingBase::BeginPlay()
{
	Super::BeginPlay();
}

void AItemBuildingBase::RefreshNearbyInventories()
{
	TArray<UBuildSlotComponent*> Slots;
	Slot->GetOwner()->GetComponents<UBuildSlotComponent>(Slots);

	NearbyInventories.Empty();

	for (const UBuildSlotComponent* NearbySlot : Slots)
	{
		if (NearbySlot == Slot) continue;
		AWarehouseBuildingBase* Warehouse = Cast<AWarehouseBuildingBase>(NearbySlot->CurrentBuilding);
		if (!Warehouse) continue;
		NearbyInventories.Add(Warehouse->InventoryComponent);
	}
	
	NearbyInventories.Sort([this](const UInventoryComponent& A, const UInventoryComponent& B)
	{
		return A.Character->GetSquaredDistanceTo(this) < B.Character->GetSquaredDistanceTo(this);
	});
}

bool AItemBuildingBase::TryFlush()
{
	RefreshNearbyInventories();

	for (UInventoryComponent* InvComp : NearbyInventories)
	{
		const TSubclassOf<UItem>& Filter = Cast<AWarehouseBuildingBase>(InvComp->GetOwner())->Filter;
		
		TArray<FGameplayTag> InventoryIds;

		for (const FInventory& Inventory : InvComp->Inventories)
		{
			InventoryIds.Add(Inventory.InventoryIdentifier);
		}

		for (const FGameplayTag& Id : InventoryIds)
		{
			TArray<UItem*>& Items = Buffer->GetInventory(FGameplayTag())->Items;
			for (uint16 i = 0; i < Items.Num(); i++)
			{
				if (Filter.Get() != nullptr && Filter.Get() != Items[i]->GetClass()) continue;
				if (Items[i]->GetClass() == UItem::StaticClass()) continue;
				Buffer->MoveItemAnySlot(InvComp, Id, FGameplayTag(), i);
			}
		}
	}

	for (const UItem* Item : Buffer->GetItems(FGameplayTag()))
	{
		if (Item->GetClass() != Buffer->GetInventory(FGameplayTag())->EmptyItemClass) return false;
	}
	
	return true;
}

void AItemBuildingBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority()) return;

	if (!IsOutputLocked()) return;

	if (TryFlush())
	{
		bOutputLocked = false;
	}
}
