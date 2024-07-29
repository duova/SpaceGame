﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

struct FItemDescriptor;
class UInputPayload;
class AGameCharacter;
class UItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemUpdate);

USTRUCT(BlueprintType)
struct FInventory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InventoryIdentifier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UItem*> Items;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0), BlueprintReadOnly)
	int32 Capacity;

	//Input bindings for the item slots of this inventory.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UInputAction*> OrderedInputBindings;

	//True to bind inputs to active abilities.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bBindInputs;

	//True to allow passive and active abilities to work.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCanUseAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UItem> EmptyItemClass;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template <>
struct TStructOpsTypeTraits<FInventory> : public TStructOpsTypeTraitsBase2<FInventory>
{
	enum
	{
		WithNetSerializer = true
	};
};

/**
 * Component that can hold multiple inventories.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMECORE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY()
	AGameCharacter* Character;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetItemOwnerNotChecked(FGameplayTag InventoryIdentifier, int32 Index, FInventory& Inventory);

	//Index from GetItems. Returns null if type doesn't exist, index is outside of inventory capacity, or if slot is filled.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UItem* AddItem(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass, const int32 Count,
	                      const int32 Index);

	//Index from GetItems. Returns null if type doesn't exist, index is outside of inventory capacity, or if all slots are filled.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool AddItemAnySlot(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass,
	                           const int32 Count, const bool bStackIfPossible = true);

	//Index from GetItems. Returns false if type doesn't exist, index is invalid, or the slot is empty. Count 0 removes the whole stack.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveItem(const FGameplayTag InventoryIdentifier, const int32 Index, const int32 Count = 0);

	//Attempts to move an item within this UInventoryComponent (LOCAL) to a specified index of any inventory (OTHER).
	//Swaps instead if an item already exists there. Optionally stacks LOCAL on OTHER if they're the same class.
	//Index from GetItems. Returns false if the index, inventories, or the inventory component are invalid.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool MoveOrSwapItem(UInventoryComponent* OtherInventoryComponent, const FGameplayTag OtherInventoryIdentifier,
	                    const int32 OtherItemIndex, const FGameplayTag LocalInventoryIdentifier,
	                    const int32 LocalItemIndex, const bool bStackIfPossible = true);

	//Attempts to move an item within this UInventoryComponent (LOCAL) to anywhere in another inventory (OTHER).
	//Returns false if the index, inventories, or the inventory component are invalid, or if there's no space.
	//Will return false if only a portion of the stack can be moved, but will still do so.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool MoveItemAnySlot(UInventoryComponent* OtherInventoryComponent, const FGameplayTag OtherInventoryIdentifier,
	                     const FGameplayTag LocalInventoryIdentifier,
	                     const int32 LocalItemIndex, const bool bStackIfPossible = true);

	//Removes requested items, only if they are all available.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveItemsBatched(const TArray<FItemDescriptor>& Items);

	//Returns the inventory as an array of items including the items representing empty slots. Empty if not found.
	UFUNCTION(BlueprintPure)
	TArray<UItem*> GetItems(const FGameplayTag InventoryIdentifier) const;

	UFUNCTION(BlueprintPure)
	int32 GetIndex(UItem* Item, const FGameplayTag InventoryIdentifier);

	//Returns the items of all arrays NOT including the items representing empty slots. Empty if not found.
	UFUNCTION(BlueprintPure)
	TArray<UItem*> GetItemsInAllInventories() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool IncreaseCapacity(const FGameplayTag InventoryIdentifier, const int32 Count);

	UPROPERTY(BlueprintAssignable)
	FOnItemUpdate OnItemUpdate;

	FInventory* GetInventory(const FGameplayTag InventoryIdentifier);

	void OnInput(const UInputAction* Input, const UInputPayload* InputData, FGameplayTag UItem::* EventTag);

	void OnInputDown(const UInputAction* Input, const UInputPayload* InputData);

	void OnInputUp(const UInputAction* Input, const UInputPayload* InputData);

	TMap<FGameplayAbilitySpecHandle, UItem*> AbilityRegistry;

	UPROPERTY(Replicated, EditAnywhere, ReplicatedUsing = InternalOnItemUpdate)
	TArray<FInventory> Inventories;
	
	static TMap<const TSubclassOf<UItem>, int32> GetJoinedRequirements(const TArray<FItemDescriptor>& Items);

protected:
	bool RegisterAbilities(UItem* Item, const FInventory& Inventory);

	bool UnregisterAbilities(UItem* Item);

private:
	void InternalMoveOrSwapItems(const UInventoryComponent* OtherInventoryComponent, int32 OtherItemIndex,
	                             int32 LocalItemIndex,
	                             UItem* Other, FInventory* OtherInventory, UItem* Local, FInventory* LocalInventory);

	UFUNCTION()
	void InternalOnItemUpdate() const;
};
