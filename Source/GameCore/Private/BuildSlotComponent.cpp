// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildSlotComponent.h"

#include "Building.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UBuildSlotComponent::UBuildSlotComponent(): CurrentBuilding(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBuildSlotComponent::Enable()
{
	bEnabled = true;
	ChangeBuilding(DefaultBuilding);
}

void UBuildSlotComponent::Disable()
{
	bEnabled = false;
	if (CurrentBuilding)
	{
		CurrentBuilding->Destroy();
		MARK_PROPERTY_DIRTY_FROM_NAME(UBuildSlotComponent, CurrentBuilding, this);
	}
}

void UBuildSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	Enable();
}

void UBuildSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentBuilding)
	{
		CurrentBuilding->Destroy(true);
		MARK_PROPERTY_DIRTY_FROM_NAME(UBuildSlotComponent, CurrentBuilding, this);
	}
}

void UBuildSlotComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuildSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UBuildSlotComponent, CurrentBuilding, RepParams);
}

bool UBuildSlotComponent::ChangeBuilding(const TSubclassOf<ABuilding>& BuildingClass)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (!bEnabled) return false;

	if (CurrentBuilding)
	{
		CurrentBuilding->Destroy(true);
	}

	CurrentBuilding = GetWorld()->SpawnActorDeferred<ABuilding>(BuildingClass, GetComponentTransform());
	CurrentBuilding->Slot = this;
	UGameplayStatics::FinishSpawningActor(CurrentBuilding, GetComponentTransform());

	if (CurrentBuilding)
	{
		CurrentBuilding->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget,
																		   EAttachmentRule::SnapToTarget,
																		   EAttachmentRule::SnapToTarget, true));
	}
	
	MARK_PROPERTY_DIRTY_FROM_NAME(ABuilding, Slot, CurrentBuilding);
	MARK_PROPERTY_DIRTY_FROM_NAME(UBuildSlotComponent, CurrentBuilding, this);

	return true;
}

bool UBuildSlotComponent::PurchaseBuilding(const TSubclassOf<ABuilding>& BuildingClass, UInventoryComponent* Source)
{
	if (!GetOwner()->HasAuthority()) return false;

	ABuilding* CDO = BuildingClass->GetDefaultObject<ABuilding>();
	if (CDO->Tiers.Num() == 0) return false;
	if (!Source->RemoveItemsBatched(CDO->Tiers[0].Cost)) return false;

	ChangeBuilding(BuildingClass);

	return true;
}
