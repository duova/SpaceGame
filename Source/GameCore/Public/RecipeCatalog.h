// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RecipeCatalog.generated.h"

class ABuilding;
class UItem;

USTRUCT(BlueprintType)
struct GAMECORE_API FItemDescriptor
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UItem> ItemClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0), BlueprintReadOnly)
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDescriptor> Items;
};

UCLASS(BlueprintType)
class GAMECORE_API UBuildingRecipeResult : public URecipeResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABuilding> BuildingClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 BuildingTier;
};

USTRUCT(BlueprintType)
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDescriptor> ResearchCost;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDescriptor> Inputs;
	
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0), BlueprintReadOnly)
	float RecipeBaseTime;

	//Only items support multi-result. Only the building at index 0 will be used.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly)
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
