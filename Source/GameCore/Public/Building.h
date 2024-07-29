﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

class UInventoryComponent;
class UItem;
struct FItemDescriptor;
class UBuildSlotComponent;

USTRUCT(BlueprintType)
struct FTierInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TierDisplayName;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere)
	UAnimMontage* IdleAnimation;

	//Animation when upgrading to this tier.
	UPROPERTY(EditAnywhere)
	UAnimMontage* UpgradeAnimation;

	UPROPERTY(EditAnywhere)
	TArray<FItemDescriptor> Cost;
};

UCLASS(Abstract, Blueprintable)
class GAMECORE_API ABuilding : public AActor
{
	GENERATED_BODY()

public:
	ABuilding();

	UPROPERTY(Replicated, BlueprintReadOnly)
	UBuildSlotComponent* Slot;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTierInfo> Tiers;
	
	UFUNCTION(BlueprintPure)
	int32 GetTier() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool PurchaseUpgrade(UInventoryComponent* Source);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ChangeTier(int32 NewTier);

	UFUNCTION(Reliable, NetMulticast)
	void MulticastChangeTier(int32 NewTier);

protected:
	virtual void BeginPlay() override;

	virtual void OnChangeTier();
	
	void HandleAwaitingUpgradeAnimationOnTick();

	//Synced by reliable upgrade RPC.
	int32 Tier;

	UPROPERTY()
	USkeletalMeshComponent* Skm;

	bool bAwaitingUpgradeAnimation = false;
public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly)
	ABuilding* UpgradeLockedBy = nullptr;

	UFUNCTION(BlueprintPure)
	bool CanUpgrade() const;

};
