// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "UObject/Object.h"
#include "Item.generated.h"

class UInventoryComponent;
/**
 * Stackable item.
 */
UCLASS(Blueprintable)
class GAMECORE_API UItem : public UObject
{
	GENERATED_BODY()

public:
	UItem();

	virtual bool IsSupportedForNetworking() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetMaxStackSize() const;

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	UTexture2D* Icon;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 Count;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> ItemAbilities;

	UPROPERTY(Replicated)
	TArray<FGameplayAbilitySpecHandle> OrderedAbilityHandles;

	UPROPERTY(EditAnywhere)
	FGameplayTag OnDownEvent;

	UPROPERTY(EditAnywhere)
	FGameplayTag OnUpEvent;

	UPROPERTY(BlueprintReadOnly, Replicated)
	UInventoryComponent* OwningInvComp;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FGameplayTag OwningInvIdentifier;

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 OwningInvIndex;
	
protected:
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 MaxStackSize;
};
