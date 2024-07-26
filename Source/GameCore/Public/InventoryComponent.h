// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemUpdate);

USTRUCT()
struct FInventory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag InventoryIdentifier;
	
	UPROPERTY(VisibleAnywhere)
	TArray<UItem*> Items;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 Capacity;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UItem> EmptyItemClass;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
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

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Index from GetItems. Returns null if type doesn't exist, index is outside of inventory capacity, or if slot is filled.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UItem* AddToInventory(const FGameplayTag InventoryIdentifier, const TSubclassOf<UItem> ItemClass, const int32 Count, const int32 Index);

	//Index from GetItems. Returns false if type doesn't exist, index is invalid, or the slot is empty.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveFromInventory(const FGameplayTag InventoryIdentifier, const int32 Index);

	//Attempts to move an item within this UInventoryComponent (LOCAL) to a specified index of any inventory (OTHER).
	//Swaps instead if an item already exists there. Optionally stacks LOCAL on OTHER if they're the same class.
	//Index from GetItems. Returns false if the index, inventories, or the inventory component are invalid.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool MoveOrSwapItem(UInventoryComponent* OtherInventoryComponent, const FGameplayTag OtherInventoryIdentifier,
	                     const int32 OtherItemIndex, const FGameplayTag LocalInventoryIdentifier,
	                     const int32 LocalItemIndex, const bool bStackIfPossible = true);
	
	//Returns the inventory as an array of items including the items representing empty slots. Empty if not found.
	UFUNCTION(BlueprintPure)
	TArray<UItem*> GetItems(const FGameplayTag InventoryIdentifier) const;

	UPROPERTY(BlueprintAssignable)
	FOnItemUpdate OnItemUpdate;

protected:
	UPROPERTY(Replicated, EditAnywhere, ReplicatedUsing = InternalOnItemUpdate)
	TArray<FInventory> Inventories;

private:
	void InternalMoveOrSwapItems(const UInventoryComponent* OtherInventoryComponent, int32 OtherItemIndex,
	                             int32 LocalItemIndex,
	                             UItem* Other, FInventory* OtherInventory, UItem* Local, FInventory* LocalInventory);

	UFUNCTION()
	void InternalOnItemUpdate() const;
};
