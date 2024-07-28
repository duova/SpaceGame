// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RecipeCatalog.generated.h"

class ABuilding;
class UItem;

UCLASS(EditInlineNew)
class GAMECORE_API UItemDescriptor : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UItem> ItemClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 ItemCount;
};

UCLASS(Abstract, EditInlineNew)
class GAMECORE_API URecipeResult : public UObject
{
	GENERATED_BODY()
};

UCLASS()
class GAMECORE_API UItemRecipeResult : public URecipeResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Instanced)
	TArray<UItemDescriptor*> Items;
};

UCLASS()
class GAMECORE_API UBuildingRecipeResult : public URecipeResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABuilding> BuildingClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 BuildingTier;
};

USTRUCT()
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	UTexture2D* Icon;
	
	UPROPERTY(EditAnywhere, Instanced)
	TArray<UItemDescriptor*> ResearchCost;
	
	UPROPERTY(EditAnywhere, Instanced)
	TArray<UItemDescriptor*> Inputs;
	
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	float RecipeBaseTime;
	
	UPROPERTY(EditAnywhere, Instanced)
	TArray<URecipeResult*> Result;
};


/**
 * 
 */
UCLASS()
class GAMECORE_API URecipeCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FRecipe> Recipes;
};
