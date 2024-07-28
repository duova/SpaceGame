// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameGi.generated.h"

class URecipeCatalog;

/**
 * Must be extended in BP to add the RecipeCatalog.
 */
UCLASS(EditInlineNew)
class GAMECORE_API UGameGi : public UGameInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	URecipeCatalog* RecipeCatalog;
	
	TArray<bool> RecipeUnlocks;
};
