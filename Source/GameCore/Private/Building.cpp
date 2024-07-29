// Fill out your copyright notice in the Description page of Project Settings.


#include "Building.h"

#include "Net/UnrealNetwork.h"


ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	Skm = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
}

int32 ABuilding::GetTier() const
{
	return Tier;
}

bool ABuilding::ChangeTier(const int32 NewTier)
{
	if (!HasAuthority()) return false;
	if (NewTier >= Tiers.Num() || NewTier < 0) return false;
	MulticastChangeTier(Tier);
	return true;
}

void ABuilding::MulticastChangeTier_Implementation(const int32 NewTier)
{
	Tier = NewTier;
	if (GetNetMode() == NM_DedicatedServer) return;
	DisplayName = Tiers[Tier].TierDisplayName;
	if (Skm && Skm->GetAnimInstance())
	{
		Skm->SetSkinnedAssetAndUpdate(Tiers[Tier].SkeletalMesh);
		Skm->GetAnimInstance()->Montage_Play(Tiers[Tier].UpgradeAnimation);
		bAwaitingUpgradeAnimation = true;
	}
	OnChangeTier();
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	ChangeTier(0);
}

void ABuilding::OnChangeTier()
{
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

