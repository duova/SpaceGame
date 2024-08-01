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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon;

	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnRep_Count)
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayAbility>> ItemAbilities;

	UPROPERTY(Replicated)
	TArray<FGameplayAbilitySpecHandle> OrderedAbilityHandles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag OnDownEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag OnUpEvent;

	UPROPERTY(BlueprintReadOnly, Replicated)
	UInventoryComponent* OwningInvComp;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FGameplayTag OwningInvIdentifier;

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 OwningInvIndex;

	UPROPERTY(EditAnywhere)
	bool bDoNotRegister = false;
	
protected:
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1), BlueprintReadOnly)
	int32 MaxStackSize = 1;

	UFUNCTION()
	void OnRep_Count() const;
};
