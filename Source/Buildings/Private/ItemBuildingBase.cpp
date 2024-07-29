// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBuildingBase.h"

#include "BuildSlotComponent.h"
#include "InventoryComponent.h"
#include "Item.h"
#include "RecipeCatalog.h"
#include "WarehouseBuildingBase.h"

AItemBuildingBase::AItemBuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1;
	Buffer = CreateDefaultSubobject<UInventoryComponent>("Buffer");
	FInventory BufferInventory;
	BufferInventory.Capacity = 100;
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

	const TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements = GetJoinedRequirements(Items);

	return InternalHasItems(JoinedItemRequirements);
}

TMap<const TSubclassOf<UItem>, int32> AItemBuildingBase::GetJoinedRequirements(const TArray<FItemDescriptor>& Items)
{
	TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements;
	for (const FItemDescriptor& Descriptor : Items)
	{
		if (!JoinedItemRequirements.Contains(Descriptor.ItemClass))
		{
			JoinedItemRequirements.Add(Descriptor.ItemClass, Descriptor.ItemCount);
		}
		else
		{
			JoinedItemRequirements[Descriptor.ItemClass] += Descriptor.ItemCount;
		}
	}
	return JoinedItemRequirements;
}

bool AItemBuildingBase::InputItems(const TArray<FItemDescriptor>& Items, const bool bAssumeEnoughItems)
{
	RefreshNearbyInventories();

	TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements = GetJoinedRequirements(Items);
	
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

	for (UBuildSlotComponent* NearbySlot : Slots)
	{
		if (NearbySlot == Slot) continue;
		AWarehouseBuildingBase* Warehouse = Cast<AWarehouseBuildingBase>(NearbySlot->CurrentBuilding);
		if (!Warehouse) continue;
		NearbyInventories.Add(Warehouse->InventoryComponent);
	}
}

bool AItemBuildingBase::TryFlush()
{
	bool bSuccess = true;
	
	RefreshNearbyInventories();

	for (UInventoryComponent* InvComp : NearbyInventories)
	{
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
				if (Items[i]->GetClass() == UItem::StaticClass()) continue;
				bSuccess &= InvComp->MoveItemAnySlot(InvComp, Id, FGameplayTag(), i);
			}
		}
	}

	return bSuccess;
}

void AItemBuildingBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsOutputLocked()) return;

	if (TryFlush())
	{
		bOutputLocked = false;
	}
}
