// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameGs.generated.h"

class UItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRepUnlocks);

USTRUCT(BlueprintType)
struct GAMECORE_API FItemDescriptor
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UItem> ItemClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1), BlueprintReadOnly)
	int32 ItemCount;

	FItemDescriptor();

	FItemDescriptor(const TSubclassOf<UItem>& InItemClass, int32 InItemCount);
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

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0), BlueprintReadOnly)
	float ResearchBaseTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDescriptor> Result;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUnlocked;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	FRecipe();
};

template <>
struct TStructOpsTypeTraits<FRecipe> : public TStructOpsTypeTraitsBase2<FRecipe>
{
	enum
	{
		WithNetSerializer = true
	};
};

/**
 * 
 */
UCLASS()
class GAMECORE_API AGameGs : public AGameState
{
	GENERATED_BODY()

	AGameGs();
	void RegisterItemClasses();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, ReplicatedUsing = OnRep_Recipes)
	TArray<FRecipe> Recipes;

	void MarkRecipesDirty() const;

	UPROPERTY(BlueprintAssignable)
	FOnRepUnlocks OnRecipeUnlocksUpdated;
	
	UFUNCTION()
	void OnRep_Recipes() const;

	UPROPERTY(EditAnywhere)
	FString ItemSearchPath;

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<UItem>> ItemRegistry;
};
