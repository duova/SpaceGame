// Fill out your copyright notice in the Description page of Project Settings.


#include "OutputBuildingBase.h"

#include "BuildSlotComponent.h"
#include "InventoryComponent.h"
#include "Item.h"
#include "RecipeCatalog.h"
#include "WarehouseBuildingBase.h"

AOutputBuildingBase::AOutputBuildingBase()
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

bool AOutputBuildingBase::OutputItems(const TArray<UItemDescriptor>& Items)
{
	if (IsOutputLocked()) return false;
	for (const UItemDescriptor& Item : Items)
	{
		Buffer->AddToInventoryAnySlot(FGameplayTag(), Item.ItemClass, Item.ItemCount);
	}
	bOutputLocked = true;
	if (TryFlush())
	{
		bOutputLocked = false;
	}
	return true;
}

bool AOutputBuildingBase::IsOutputLocked() const
{
	return bOutputLocked;
}

void AOutputBuildingBase::BeginPlay()
{
	Super::BeginPlay();
}

bool AOutputBuildingBase::TryFlush()
{
	bool bSuccess = true;
	
	TArray<UBuildSlotComponent*> Slots;
	Slot->GetOwner()->GetComponents<UBuildSlotComponent>(Slots);

	OutputInventories.Empty();

	for (UBuildSlotComponent* NearbySlot : Slots)
	{
		if (NearbySlot == Slot) continue;
		AWarehouseBuildingBase* Warehouse = Cast<AWarehouseBuildingBase>(NearbySlot->CurrentBuilding);
		if (!Warehouse) continue;
		OutputInventories.Add(Warehouse->InventoryComponent);
	}

	for (UInventoryComponent* InvComp : OutputInventories)
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
				bSuccess &= InvComp->MoveToInventory(InvComp, Id, FGameplayTag(), i);
			}
		}
	}

	return bSuccess;
}

void AOutputBuildingBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsOutputLocked()) return;

	if (TryFlush())
	{
		bOutputLocked = false;
	}
}
