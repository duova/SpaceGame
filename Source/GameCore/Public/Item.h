// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Item.generated.h"

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

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 Count;
	
protected:
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 MaxStackSize;
};
