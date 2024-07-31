// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "GameCharacter.h"

AGamePlayerController::AGamePlayerController()
{
	InputPayload = CreateDefaultSubobject<UInputPayload>("InputPayload");
}

void AGamePlayerController::ServerDropDraggedItem_Implementation(UItem* Dropped, UItem* Current)
{
	Dropped->OwningInvComp->DropDraggedItem(Dropped, Current);
}

void AGamePlayerController::ServerSplitItem_Implementation(UItem* Item)
{
	Item->OwningInvComp->SplitItem(Item);
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
