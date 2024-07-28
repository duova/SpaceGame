// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "GameCharacter.h"
#include "GameCore.h"
#include "GamePlayerController.h"
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
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
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

		//Fill with empty items.
		for (FInventory& Inventory : Inventories)
		{
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

void UInventoryComponent::SetItemOwnerNotChecked(const FGameplayTag InventoryIdentifier, const int32 Index, FInventory& Inventory)
{
	Inventory.Items[Index]->OwningInvComp = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvComp, Inventory.Items[Index]);
	Inventory.Items[Index]->OwningInvIdentifier = InventoryIdentifier;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIdentifier, Inventory.Items[Index]);
	Inventory.Items[Index]->OwningInvIndex = Index;
	MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OwningInvIndex, Inventory.Items[Index]);
}

UItem* UInventoryComponent::AddToInventory(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass,
                                           const int32 Count, const int32 Index)
{
	if (!GetOwner()->HasAuthority()) return nullptr;
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
			RegisterAbilities(Inventory.Items[Index], Inventory);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory.Items[Index]);
			SetItemOwnerNotChecked(InventoryIdentifier, Index, Inventory);
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

bool UInventoryComponent::AddToInventoryAnySlot(const FGameplayTag InventoryIdentifier,
	const TSubclassOf<UItem> ItemClass, const int32 Count, const bool bStackIfPossible)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count < 1) return false;

	int32 MutableCount = Count;

	TArray<UItem*> Items = GetItems(InventoryIdentifier);

	if (bStackIfPossible)
	{
		for (UItem* Item : Items)
		{
			if (Item->GetClass() != ItemClass) continue;
			const int32 TransferableCount = FMath::Min(Item->GetMaxStackSize() - Item->Count, Count);
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
			AddToInventory(InventoryIdentifier, ItemClass, MutableCount, i);
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::RemoveFromInventory(const FGameplayTag InventoryIdentifier, const int32 Index,
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
				return true;
			}
			UnregisterAbilities(Inventory.Items[Index]);
			RemoveReplicatedSubObject(Inventory.Items[Index]);
			Inventory.Items[Index] = NewObject<UItem>(this, Inventory.EmptyItemClass);
			AddReplicatedSubObject(Inventory.Items[Index]);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Inventory.Items[Index]);
			RegisterAbilities(Inventory.Items[Index], Inventory);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, OrderedAbilityHandles, Inventory.Items[Index]);
			SetItemOwnerNotChecked(InventoryIdentifier, Index, Inventory);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventories, this);
			return true;
		}
	}

	if (GetOwner()->HasAuthority())
	{
		InternalOnItemUpdate();
		//Client side called OnRep.
	}

	return false;
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
	for (int32 i = Item->OrderedAbilityHandles.Num(); i >= 0; i--)
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

void UInventoryComponent::InternalMoveOrSwapItems(const UInventoryComponent* OtherInventoryComponent,
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

void UInventoryComponent::InternalOnItemUpdate() const
{
	if (OnItemUpdate.IsBound()) OnItemUpdate.Broadcast();
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
		Other->Count += Local->Count;
		MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Other);
		RemoveFromInventory(LocalInventoryIdentifier, LocalItemIndex);
	}

	InternalMoveOrSwapItems(OtherInventoryComponent, OtherItemIndex, LocalItemIndex, Other, OtherInventory, Local,
	                        LocalInventory);

	SetItemOwnerNotChecked(LocalInventoryIdentifier, LocalItemIndex, *LocalInventory);
	SetItemOwnerNotChecked(OtherInventoryIdentifier, OtherItemIndex, *OtherInventory);
	

	if (GetOwner()->HasAuthority())
	{
		InternalOnItemUpdate();
		//Client side called OnRep.
	}

	return true;
}

bool UInventoryComponent::MoveToInventory(UInventoryComponent* OtherInventoryComponent,
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
			LocalItem -= TransferableCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, Item);
			MARK_PROPERTY_DIRTY_FROM_NAME(UItem, Count, LocalItem);
			if (LocalItem->Count <= 0)
			{
				RemoveFromInventory(LocalInventoryIdentifier, LocalItemIndex);
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
                                  FGameplayTag UItem::*EventTag)
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
			for (const FGameplayAbilitySpecHandle Handle : Item->OrderedAbilityHandles)
			{
				Asc->TriggerAbilityFromGameplayEvent(Handle, Asc->AbilityActorInfo.Get(), Inventory.Items[i]->*EventTag, &Data, *Asc);
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
