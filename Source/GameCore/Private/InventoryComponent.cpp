// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "GameCharacter.h"
#include "GameCore.h"
#include "GameGs.h"
#include "GamePlayerController.h"
#include "Item.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

bool FInventory::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << InventoryIdentifier;
	Ar << Capacity;
	Ar << EmptyItemClass;
	Ar << Items;

	return bOutSuccess;
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AGameCharacter>(GetOwner());

	if (GetOwner()->HasAuthority())
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

		for (FInventory& Inventory : Inventories)
		{
			//Fill with empty items.
			for (uint16 i = 0; i < Inventory.Capacity; i++)
			{
				const uint16 Index = Inventory.Items.Emplace(NewObject<UItem>(this, Inventory.EmptyItemClass));
				AddReplicatedSubObject(Inventory.Items[Index]);
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory.Items[Index]);
				RegisterAbilities(Inventory.Items[Index], Inventory);
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory.Items[Index]);
				Inventory.Items[Index]->OwningInvComp = this;
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvComp, Inventory.Items[Index]);
				Inventory.Items[Index]->OwningInvIdentifier = Inventory.InventoryIdentifier;
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIdentifier, Inventory.Items[Index]);
				Inventory.Items[Index]->OwningInvIndex = Index;
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIndex, Inventory.Items[Index]);
			}
			//Insert starting items.
			for (const FItemDescriptor& Item : Inventory.StartingItems)
			{
				if (!Item.ItemClass.Get()) continue;
				AddItemAnySlot(Inventory.InventoryIdentifier, Item.ItemClass, Item.ItemCount);
			}
		}

		MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
	}
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPendingItemUpdateBroadcast)
	{
		if (OnItemUpdate.IsBound())
		{
			OnItemUpdate.Broadcast();
		}
		bPendingItemUpdateBroadcast = false;
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, Inventories, RepParams);
}

void UInventoryComponent::SetItemOwnerNotChecked(const FGameplayTag InventoryIdentifier, const int32 Index,
                                                 FInventory& Inventory, UInventoryComponent* InvComp)
{
	Inventory.Items[Index]->OwningInvComp = InvComp;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvComp, Inventory.Items[Index]);
	Inventory.Items[Index]->OwningInvIdentifier = InventoryIdentifier;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIdentifier, Inventory.Items[Index]);
	Inventory.Items[Index]->OwningInvIndex = Index;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIndex, Inventory.Items[Index]);
}

UItem* UInventoryComponent::AddItem(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass,
                                    const int32 Count, const int32 Index)
{
	if (!GetOwner()->HasAuthority()) return nullptr;
	if (Count < 1) return nullptr;
	if (!ItemClass.Get()) return nullptr;
	if (Count > ItemClass.GetDefaultObject()->GetMaxStackSize()) return nullptr;
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
			RegisterAbilities(Inventory.Items[Index], Inventory);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory.Items[Index]);
			SetItemOwnerNotChecked(InventoryIdentifier, Index, Inventory, this);
			InternalOnItemUpdate();
			//Client side called OnRep.
			return Inventory.Items[Index];
		}
	}

	return nullptr;
}

bool UInventoryComponent::AddItemAnySlot(const FGameplayTag InventoryIdentifier,
                                         const TSubclassOf<UItem> ItemClass, const int32 Count,
                                         const bool bStackIfPossible)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count < 1) return false;
	if (!ItemClass.Get()) return false;
	if (!GetInventory(InventoryIdentifier)) return false;

	int32 MutableCount = Count;

	TArray<UItem*> Items = GetItems(InventoryIdentifier);

	if (bStackIfPossible)
	{
		for (UItem* Item : Items)
		{
			if (Item->GetClass() != ItemClass) continue;
			const int32 TransferableCount = FMath::Min(Item->GetMaxStackSize() - Item->Count, MutableCount);
			Item->Count += TransferableCount;
			MutableCount -= TransferableCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Item);
			if (MutableCount <= 0)
			{
				return true;
			}
		}
	}

	for (uint16 i = 0; i < Items.Num(); i++)
	{
		if (Items[i]->GetClass() == GetInventory(InventoryIdentifier)->EmptyItemClass)
		{
			const int32 TransferableCount = FMath::Min(ItemClass.GetDefaultObject()->GetMaxStackSize(), MutableCount);
			AddItem(InventoryIdentifier, ItemClass, TransferableCount, i);
			MutableCount -= TransferableCount;
			if (MutableCount <= 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool UInventoryComponent::RemoveItem(const FGameplayTag InventoryIdentifier, const int32 Index,
                                     const int32 Count)
{
	if (!GetOwner()->HasAuthority()) return false;
	for (FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == InventoryIdentifier)
		{
			if (Index >= Inventory.Capacity) return false;
			if (Index >= Inventory.Items.Num()) return false;
			if (Inventory.Items[Index]->GetClass() == Inventory.EmptyItemClass) return false;
			if (Count > 0 && Count < Inventory.Items[Index]->Count)
			{
				Inventory.Items[Index]->Count -= Count;
				MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory.Items[Index]);
				InternalOnItemUpdate();
				//Client side called OnRep.
				return true;
			}
			UnregisterAbilities(Inventory.Items[Index]);
			RemoveReplicatedSubObject(Inventory.Items[Index]);
			Inventory.Items[Index] = NewObject<UItem>(this, Inventory.EmptyItemClass);
			AddReplicatedSubObject(Inventory.Items[Index]);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory.Items[Index]);
			RegisterAbilities(Inventory.Items[Index], Inventory);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory.Items[Index]);
			SetItemOwnerNotChecked(InventoryIdentifier, Index, Inventory, this);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
			InternalOnItemUpdate();
			//Client side called OnRep.
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::RemoveAll(const FGameplayTag InventoryIdentifier)
{
	FInventory* Inv = GetInventory(InventoryIdentifier);
	if (!Inv) return false;
	for (uint16 i = 0; i < Inv->Items.Num(); i++)
	{
		RemoveItem(InventoryIdentifier, i);
	}
	return true;
}

int32 UInventoryComponent::GetInventoryIndexByIdentifier(const FGameplayTag InventoryIdentifier)
{
	for (uint16 i = 0; i < Inventories.Num(); i++)
	{
		if (Inventories[i].InventoryIdentifier == InventoryIdentifier) return i;
	}
	return -1;
}

TMap<const TSubclassOf<UItem>, int32> UInventoryComponent::GetJoinedRequirements(const TArray<FItemDescriptor>& Items)
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

bool UInventoryComponent::RegisterAbilities(UItem* Item, const FInventory& Inventory)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!Character) return false;
	if (!Inventory.bCanUseAbilities) return false;
	UAbilitySystemComponent* Asc = Character->GetAbilitySystemComponent();
	Item->OrderedAbilityHandles.Empty();
	for (const TSubclassOf<UGameplayAbility>& Ability : Item->ItemAbilities)
	{
		if (!Ability.Get())
		{
			Item->OrderedAbilityHandles.Add(FGameplayAbilitySpecHandle());
			continue;
		}
		FGameplayAbilitySpecHandle Handle = Asc->GiveAbility(FGameplayAbilitySpec(Ability, 1, INDEX_NONE, Item));
		Item->OrderedAbilityHandles.Add(Handle);
		AbilityRegistry.Add(Handle, Item);
	}
	return true;
}

bool UInventoryComponent::UnregisterAbilities(UItem* Item)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!Character) return false;
	UAbilitySystemComponent* Asc = Character->GetAbilitySystemComponent();
	for (int32 i = Item->OrderedAbilityHandles.Num() - 1; i >= 0; i--)
	{
		if (Item->OrderedAbilityHandles[i].IsValid())
		{
			FGameplayAbilitySpecHandle Handle = Item->OrderedAbilityHandles[i];
			Asc->ClearAbility(Handle);
			AbilityRegistry.Remove(Handle);
		}
		Item->OrderedAbilityHandles.RemoveAt(i, 1, false);
	}
	Item->OrderedAbilityHandles.Shrink();
	return true;
}

void UInventoryComponent::InternalMoveOrSwapItems(UInventoryComponent* OtherInventoryComponent,
                                                  const int32 OtherItemIndex, const int32 LocalItemIndex, UItem* Other,
                                                  FInventory* OtherInventory, UItem* Local, FInventory* LocalInventory)
{
	RemoveReplicatedSubObject(LocalInventory->Items[LocalItemIndex]);
	RemoveReplicatedSubObject(OtherInventory->Items[OtherItemIndex]);

	UnregisterAbilities(LocalInventory->Items[LocalItemIndex]);
	UnregisterAbilities(OtherInventory->Items[OtherItemIndex]);

	if (Other)
	{
		LocalInventory->Items[LocalItemIndex] = Other;
	}
	else
	{
		LocalInventory->Items[LocalItemIndex] = NewObject<UItem>(this, LocalInventory->EmptyItemClass);
	}

	if (Local)
	{
		OtherInventory->Items[OtherItemIndex] = Local;
	}
	else
	{
		OtherInventory->Items[OtherItemIndex] = NewObject<UItem>(this, OtherInventory->EmptyItemClass);
	}

	RegisterAbilities(LocalInventory->Items[LocalItemIndex], *LocalInventory);
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, LocalInventory->Items[LocalItemIndex]);
	RegisterAbilities(OtherInventory->Items[OtherItemIndex], *OtherInventory);
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, OtherInventory->Items[OtherItemIndex]);

	AddReplicatedSubObject(LocalInventory->Items[LocalItemIndex]);
	AddReplicatedSubObject(OtherInventory->Items[OtherItemIndex]);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, OtherInventoryComponent);
}

void UInventoryComponent::InternalOnItemUpdate()
{
	bPendingItemUpdateBroadcast = true;
}

bool UInventoryComponent::MoveOrSwapItem(UInventoryComponent* OtherInventoryComponent,
                                         const FGameplayTag OtherInventoryIdentifier, const int32 OtherItemIndex,
                                         const FGameplayTag LocalInventoryIdentifier,
                                         const int32 LocalItemIndex, const bool bStackIfPossible)
{
	if (!GetOwner()->HasAuthority()) return false;
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
		const uint16 TransferableCount = FMath::Min(Other->GetMaxStackSize() - Other->Count, Local->Count);
		Other->Count += TransferableCount;
		MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Other);
		RemoveItem(LocalInventoryIdentifier, LocalItemIndex, TransferableCount);
		InternalOnItemUpdate();
		OtherInventoryComponent->InternalOnItemUpdate();
		//Client side calls OnRep.
		return true;
	}

	InternalMoveOrSwapItems(OtherInventoryComponent, OtherItemIndex, LocalItemIndex, Other, OtherInventory, Local,
	                        LocalInventory);

	SetItemOwnerNotChecked(LocalInventoryIdentifier, LocalItemIndex, *LocalInventory, this);
	SetItemOwnerNotChecked(OtherInventoryIdentifier, OtherItemIndex, *OtherInventory, OtherInventoryComponent);

	InternalOnItemUpdate();
	OtherInventoryComponent->InternalOnItemUpdate();
	//Client side called OnRep.

	return true;
}

void UInventoryComponent::DropDraggedItem(UItem* Dropped, UItem* Current)
{
	if (Dropped == Current) return;
	Dropped->OwningInvComp->MoveOrSwapItem(Current->OwningInvComp, Current->OwningInvIdentifier,
	                                       Current->OwningInvIndex, Dropped->OwningInvIdentifier,
	                                       Dropped->OwningInvIndex);
}

void UInventoryComponent::SplitItem(UItem* Item)
{
	if (Item->Count < 2) return;
	FInventory* Inv = Item->OwningInvComp->GetInventory(Item->OwningInvIdentifier);
	UInventoryComponent* OwningInvComp = Item->OwningInvComp;
	if (!Inv) return;
	for (const UItem* It : Inv->Items)
	{
		if (It->GetClass() != Inv->EmptyItemClass) continue;
		const int32 LargerHalf = FMath::DivideAndRoundUp(Item->Count, 2);
		OwningInvComp->RemoveItem(Item->OwningInvIdentifier, Item->OwningInvIndex, LargerHalf);
		OwningInvComp->AddItem(Item->OwningInvIdentifier, Item->GetClass(), LargerHalf, It->OwningInvIndex);
		return;
	}
}

bool UInventoryComponent::MoveItemAnySlot(UInventoryComponent* OtherInventoryComponent,
                                          const FGameplayTag OtherInventoryIdentifier,
                                          const FGameplayTag LocalInventoryIdentifier,
                                          const int32 LocalItemIndex, const bool bStackIfPossible)
{
	if (!GetOwner()->HasAuthority()) return false;
	FInventory* OtherInventory = OtherInventoryComponent->GetInventory(OtherInventoryIdentifier);

	if (!OtherInventory) return false;

	FInventory* LocalInventory = GetInventory(LocalInventoryIdentifier);

	UItem* LocalItem = LocalInventory && LocalInventory->Items.Num() > LocalItemIndex
		                   ? LocalInventory->Items[LocalItemIndex]
		                   : nullptr;

	if (!LocalItem) return false;

	if (bStackIfPossible)
	{
		for (UItem* Item : OtherInventory->Items)
		{
			if (Item->GetClass() != LocalItem->GetClass()) continue;
			const uint16 TransferableCount = FMath::Min(Item->GetMaxStackSize() - Item->Count, LocalItem->Count);
			Item->Count += TransferableCount;
			LocalItem->Count -= TransferableCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Item);
			OtherInventoryComponent->InternalOnItemUpdate();
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, LocalItem);
			InternalOnItemUpdate();
			if (LocalItem->Count <= 0)
			{
				RemoveItem(LocalInventoryIdentifier, LocalItemIndex);
				return true;
			}
		}
	}

	for (uint16 i = 0; i < OtherInventory->Items.Num(); i++)
	{
		if (OtherInventory->Items[i]->GetClass() == OtherInventory->EmptyItemClass)
		{
			MoveOrSwapItem(OtherInventoryComponent, OtherInventoryIdentifier, i, LocalInventoryIdentifier,
			               LocalItemIndex);
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::RemoveItemsBatched(const TArray<FItemDescriptor>& Items)
{
	TMap<const TSubclassOf<UItem>, int32> ItemReq;
	if (!InternalHasItems(Items, ItemReq)) return false;

	for (FInventory& Inventory : Inventories)
	{
		for (uint16 i = 0; i < Inventory.Items.Num(); i++)
		{
			const UItem* Item = Inventory.Items[i];
			if (!ItemReq.Contains(Item->GetClass())) continue;
			const int32 TransferCount = FMath::Min(Item->Count, ItemReq[Item->GetClass()]);
			if (TransferCount <= 0) continue;
			RemoveItem(Inventory.InventoryIdentifier, i, TransferCount);
			ItemReq[Item->GetClass()] -= TransferCount;
		}
	}

	return true;
}

void UInventoryComponent::TransferInventory(UInventoryComponent* OtherInventoryComponent,
	const FGameplayTag OtherInventoryIdentifier, const FGameplayTag LocalInventoryIdentifier,
	const bool bStackIfPossible)
{
	const FInventory* Inventory = GetInventory(LocalInventoryIdentifier);
	if (!Inventory) return;
	for (const UItem* Item : Inventory->Items)
	{
		MoveItemAnySlot(OtherInventoryComponent, OtherInventoryIdentifier, Item->OwningInvIdentifier, Item->OwningInvIndex, bStackIfPossible);
	}
}

bool UInventoryComponent::InternalHasItems(const TArray<FItemDescriptor>& Items,
                                           TMap<const TSubclassOf<UItem>, int32>& OutJoinedItemRequirements) const
{
	TMap<const TSubclassOf<UItem>, int32> JoinedItemRequirements = GetJoinedRequirements(Items);
	OutJoinedItemRequirements = JoinedItemRequirements;
	for (const UItem* Item : GetItemsInAllInventories())
	{
		if (!JoinedItemRequirements.Contains(Item->GetClass())) continue;
		JoinedItemRequirements[Item->GetClass()] -= Item->Count;
	}
	for (const TPair<const TSubclassOf<UItem>, int32>& Pair : JoinedItemRequirements)
	{
		if (Pair.Value > 0) return false;
	}
	return true;
}

bool UInventoryComponent::HasItems(const TArray<FItemDescriptor>& Items) const
{
	if (Items.Num() == 0) return true;
	TMap<const TSubclassOf<UItem>, int32> ItemReq;
	return InternalHasItems(Items, ItemReq);
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

int32 UInventoryComponent::GetIndex(UItem* Item, const FGameplayTag InventoryIdentifier)
{
	return GetInventory(InventoryIdentifier)->Items.Find(Item);
}

TArray<UItem*> UInventoryComponent::GetItemsInAllInventories() const
{
	TArray<UItem*> RetVal;
	for (const FInventory& Inventory : Inventories)
	{
		for (UItem* Item : Inventory.Items)
		{
			if (Item->GetClass() == Inventory.EmptyItemClass) continue;
			RetVal.Add(Item);
		}
	}
	return RetVal;
}

bool UInventoryComponent::IncreaseCapacity(const FGameplayTag InventoryIdentifier, const int32 Count)
{
	if (!GetOwner()->HasAuthority()) return false;
	const int32 ClampedCount = FMath::Max(0, Count);
	if (ClampedCount == 0) return false;
	FInventory* Inventory = GetInventory(InventoryIdentifier);
	if (!Inventory) return false;
	Inventory->Capacity += ClampedCount;

	for (uint16 i = 0; i < ClampedCount; i++)
	{
		const uint16 Index = Inventory->Items.Emplace(NewObject<UItem>(this, Inventory->EmptyItemClass));
		AddReplicatedSubObject(Inventory->Items[Index]);
		MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory->Items[Index]);
		RegisterAbilities(Inventory->Items[Index], *Inventory);
		MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory->Items[Index]);
		SetItemOwnerNotChecked(Inventory->InventoryIdentifier, Index, *Inventory, this);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
	InternalOnItemUpdate();
	//Client side called OnRep.
	return true;
}

TArray<FInventory> UInventoryComponent::GetInventories()
{
	return Inventories;
}

FInventory* UInventoryComponent::GetInventory(const FGameplayTag InventoryIdentifier)
{
	for (FInventory& Inventory : Inventories)
	{
		if (Inventory.InventoryIdentifier == InventoryIdentifier)
		{
			return &Inventory;
		}
	}
	return nullptr;
}

void UInventoryComponent::OnInput(const UInputAction* Input, const UInputPayload* InputData,
                                  FGameplayTag UItem::* EventTag)
{
	if (!Character) return;
	UAbilitySystemComponent* Asc = Character->GetAbilitySystemComponent();
	if (!Asc) return;
	FGameplayEventData Data;
	Data.OptionalObject = InputData;
	for (const FInventory& Inventory : Inventories)
	{
		if (!Inventory.bBindInputs) continue;
		for (uint16 i = 0; i < FMath::Min(Inventory.Items.Num(), Inventory.OrderedInputBindings.Num()); i++)
		{
			if (Inventory.OrderedInputBindings[i] != Input) continue;
			UItem* Item = Inventory.Items[i];
			Data.OptionalObject2 = Item;
			for (const FGameplayAbilitySpecHandle Handle : Item->OrderedAbilityHandles)
			{
				Asc->TriggerAbilityFromGameplayEvent(Handle, Asc->AbilityActorInfo.Get(), Inventory.Items[i]->*EventTag,
				                                     &Data, *Asc);
			}
		}
	}
}

void UInventoryComponent::OnInputDown(const UInputAction* Input, const UInputPayload* InputData)
{
	OnInput(Input, InputData, &UItem::OnDownEvent);
}

void UInventoryComponent::OnInputUp(const UInputAction* Input, const UInputPayload* InputData)
{
	OnInput(Input, InputData, &UItem::OnUpEvent);
}
