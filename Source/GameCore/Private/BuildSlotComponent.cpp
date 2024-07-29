// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildSlotComponent.h"

#include "Building.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UBuildSlotComponent::UBuildSlotComponent(): CurrentBuilding(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
}

void UBuildSlotComponent::Enable_Implementation()
{
	bEnabled = true;
	GetOwner()->SetActorEnableCollision(false);
	if (GetNetMode() == NM_DedicatedServer) return;
	GetOwner()->SetActorHiddenInGame(true);
}

void UBuildSlotComponent::Disable_Implementation()
{
	bEnabled = false;
	GetOwner()->SetActorEnableCollision(true);
	if (GetNetMode() == NM_DedicatedServer) return;
	GetOwner()->SetActorHiddenInGame(false);
}

void UBuildSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	ChangeBuilding(DefaultBuilding);
}

void UBuildSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CurrentBuilding->Destroy(true);
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

	CurrentBuilding->Destroy(true);

	CurrentBuilding = NewObject<ABuilding>(this, BuildingClass);

	CurrentBuilding->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget,
	                                                                   EAttachmentRule::SnapToTarget,
	                                                                   EAttachmentRule::SnapToTarget, false));

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
