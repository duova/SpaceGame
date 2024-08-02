// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayerController.generated.h"

class UBuildSlotComponent;
class UInventoryComponent;
class ABuilding;
class UItem;
class AGameCharacter;

UENUM(BlueprintType)
enum EUpgradeBuildingResult : uint8
{
	Success,
	MissingItems,
	AlreadyMaxLevel,
	Null
};

UCLASS()
class UInputPayload : public UObject
{
	GENERATED_BODY()

public:
	FVector AimLocation;
};

/**
 * 
 */
UCLASS()
class GAMECORE_API AGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGamePlayerController();
	
	//Global aim location as the Ai puts in the same params.
	FVector CurrentAimLocation;

	UPROPERTY(EditAnywhere)
	TArray<UInputAction*> InventoryInputs;

	UFUNCTION(BlueprintCallable, Server, Unreliable)
	void ServerDropDraggedItem(UItem* Dropped, UItem* Current);

	UFUNCTION(BlueprintCallable, Server, Unreliable)
	void ServerSplitItem(UItem* Item);

	UFUNCTION(BlueprintCallable)
	EUpgradeBuildingResult TryUpgradeBuilding(ABuilding* Building, UInventoryComponent* InvComp);

	UFUNCTION(Server, Unreliable)
	void ServerUpgradeBuilding(ABuilding* Building, UInventoryComponent* InvComp);

	UFUNCTION(BlueprintCallable)
	bool TryPurchaseBuilding(const TSubclassOf<ABuilding>& BuildingClass, UBuildSlotComponent* Slot, UInventoryComponent* InvComp);

	UFUNCTION(Server, Unreliable)
	void ServerPurchaseBuilding(TSubclassOf<ABuilding> BuildingClass, UBuildSlotComponent* Slot, UInventoryComponent* InvComp);

protected:
	virtual void SetupInputComponent() override;
	
	void OnInputDown(const UInputAction* Input);

	void OnInputUp(const UInputAction* Input);

	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	UInputPayload* InputPayload;

	UPROPERTY()
	AGameCharacter* Char;
};
