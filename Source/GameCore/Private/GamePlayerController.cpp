// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlayerController.h"

#include "AbilitySystemComponent.h"
#include "Building.h"
#include "BuildSlotComponent.h"
#include "EnhancedInputComponent.h"
#include "GameCharacter.h"

AGamePlayerController::AGamePlayerController()
{
	InputPayload = CreateDefaultSubobject<UInputPayload>("InputPayload");
}

void AGamePlayerController::ServerDropDraggedItem_Implementation(UItem* Dropped, UItem* Current)
{
	if (!Dropped) return;
	if (!Current) return;
	if (!Dropped->OwningInvComp) return;
	Dropped->OwningInvComp->DropDraggedItem(Dropped, Current);
}

void AGamePlayerController::ServerSplitItem_Implementation(UItem* Item)
{
	if (!Item) return;
	if (!Item->OwningInvComp) return;
	Item->OwningInvComp->SplitItem(Item);
}

EUpgradeBuildingResult AGamePlayerController::TryUpgradeBuilding(ABuilding* Building, UInventoryComponent* InvComp)
{
	if (!Building) return Null;
	if (!InvComp) return Null;
	const int32 UpgradeTier = Building->GetTier() + 1;
	if (UpgradeTier >= Building->Tiers.Num()) return AlreadyMaxLevel;
	if (!InvComp->HasItems(Building->Tiers[UpgradeTier].Cost)) return MissingItems;
	ServerUpgradeBuilding(Building, InvComp);
	return Success;
}

bool AGamePlayerController::TryPurchaseBuilding(const TSubclassOf<ABuilding>& BuildingClass, UBuildSlotComponent* Slot,
	UInventoryComponent* InvComp)
{
	if (!BuildingClass.Get()) return false;
	if (!Slot) return false;
	if (!InvComp) return false;
	if (BuildingClass.GetDefaultObject()->Tiers.Num() <= 0) return false;
	if (!InvComp->HasItems(BuildingClass.GetDefaultObject()->Tiers[0].Cost)) return false;
	ServerPurchaseBuilding(BuildingClass, Slot, InvComp);
	return true;
}

void AGamePlayerController::ServerPurchaseBuilding_Implementation(TSubclassOf<ABuilding> BuildingClass,
                                                                  UBuildSlotComponent* Slot, UInventoryComponent* InvComp)
{
	Slot->PurchaseBuilding(BuildingClass, InvComp);
}

void AGamePlayerController::ServerUpgradeBuilding_Implementation(ABuilding* Building, UInventoryComponent* InvComp)
{
	if (!Building) return;
	if (!InvComp) return;
	Building->PurchaseUpgrade(InvComp);
}

void AGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);

	for (const UInputAction* Input : InventoryInputs)
	{
		EnhancedInput->BindAction<AGamePlayerController, const UInputAction*>(
			Input, ETriggerEvent::Started, this, &AGamePlayerController::OnInputDown,
			Input);
		EnhancedInput->BindAction<AGamePlayerController, const UInputAction*>(
			Input, ETriggerEvent::Completed, this, &AGamePlayerController::OnInputUp,
			Input);
	}
}

void AGamePlayerController::OnInputDown(const UInputAction* Input)
{
	InputPayload->AimLocation = CurrentAimLocation;
	if (Char)
	{
		if (UInventoryComponent* InvComp = Char->GetInventoryComponent())
		{
			InvComp->OnInputDown(Input, InputPayload);
		}
	}
}

void AGamePlayerController::OnInputUp(const UInputAction* Input)
{
	InputPayload->AimLocation = CurrentAimLocation;
	if (Char)
	{
		if (UInventoryComponent* InvComp = Char->GetInventoryComponent())
		{
			InvComp->OnInputUp(Input, InputPayload);
		}
	}
}

void AGamePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Char = GetPawn<AGameCharacter>();
}
