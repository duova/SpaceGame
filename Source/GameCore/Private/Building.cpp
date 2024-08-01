// Fill out your copyright notice in the Description page of Project Settings.


#include "Building.h"

#include "GameGs.h"
#include "InventoryComponent.h"
#include "Item.h"
#include "Net/UnrealNetwork.h"


FTierInfo::FTierInfo(): SkeletalMesh(nullptr), IdleAnimation(nullptr), UpgradeAnimation(nullptr)
{
}

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SceneComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	SetRootComponent(SceneComponent);
	Skm = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
}

int32 ABuilding::GetTier() const
{
	return Tier;
}

bool ABuilding::PurchaseUpgrade(UInventoryComponent* Source)
{
	if (!HasAuthority()) return false;

	const int32 UpgradeTier = Tier + 1;
	if (UpgradeTier >= Tiers.Num()) return false;

	if (!Source->RemoveItemsBatched(Tiers[UpgradeTier].Cost)) return false;
	
	ChangeTier(UpgradeTier);

	return true;
}

bool ABuilding::ChangeTier(const int32 NewTier)
{
	if (!HasAuthority()) return false;
	if (NewTier >= Tiers.Num() || NewTier < 0) return false;
	MulticastChangeTier(NewTier);
	return true;
}

void ABuilding::MulticastChangeTier_Implementation(const int32 NewTier)
{
	if (NewTier >= Tiers.Num() || NewTier < 0) return;
	Tier = NewTier;
	if (GetNetMode() == NM_DedicatedServer)
	{
		OnChangeTier();
		return;
	}
	DisplayName = Tiers[Tier].TierDisplayName;
	if (Skm && Skm->GetAnimInstance() && Skm->GetSkeletalMeshAsset())
	{
		Skm->SetSkinnedAssetAndUpdate(Tiers[Tier].SkeletalMesh);
		if (Tiers[Tier].UpgradeAnimation != nullptr)
		{
			Skm->GetAnimInstance()->Montage_Play(Tiers[Tier].UpgradeAnimation);
		}
		bAwaitingUpgradeAnimation = true;
	}
	OnChangeTier();
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();
	
	MulticastChangeTier_Implementation(0);
}

void ABuilding::UpdateCostDisplayInventory()
{
	if (!HasAuthority()) return;
	if (!CostDisplayInventory.IsValid()) return;
	UInventoryComponent* InvComp = GetComponentByClass<UInventoryComponent>();
	if (!InvComp) return;
	const FInventory* Inv = InvComp->GetInventory(CostDisplayInventory);
	if (!Inv) return;
	const int32 UpgradeTier = Tier + 1;
	if (UpgradeTier >= Tiers.Num())
	{
		InvComp->RemoveAll(CostDisplayInventory);
	}
	else
	{
		InvComp->RemoveAll(CostDisplayInventory);
		for (const FItemDescriptor& Item : Tiers[UpgradeTier].Cost)
		{
			InvComp->AddItemAnySlot(CostDisplayInventory, Item.ItemClass, Item.ItemCount);
		}
	}
}

void ABuilding::OnChangeTier()
{
	UpdateCostDisplayInventory();

	if (OnUpdateTier.IsBound())
	{
		OnUpdateTier.Broadcast();
	}
}

void ABuilding::HandleAwaitingUpgradeAnimationOnTick()
{
	if (GetNetMode() == NM_DedicatedServer) return;
	if (!bAwaitingUpgradeAnimation || !Skm) return;
	UAnimInstance* Instance = Skm->GetAnimInstance();
	if (!Instance || Instance->Montage_IsActive(Tiers[Tier].UpgradeAnimation)) return;
	bAwaitingUpgradeAnimation = false;
	Instance->Montage_Play(Tiers[Tier].IdleAnimation);
}

void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleAwaitingUpgradeAnimationOnTick();
}

void ABuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ABuilding, Slot, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ABuilding, UpgradeLockedBy, RepParams);
}

bool ABuilding::IsMaxTier() const
{
	return Tier >= Tiers.Num() - 1;
}
