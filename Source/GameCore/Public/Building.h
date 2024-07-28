// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

class UBuildSlotComponent;

USTRUCT()
struct FTierInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText TierDisplayName;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere)
	UAnimMontage* IdleAnimation;

	//Animation when upgrading to this tier.
	UPROPERTY(EditAnywhere)
	UAnimMontage* UpgradeAnimation;
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

	UPROPERTY(EditAnywhere)
	TArray<FTierInfo> Tiers;
	
	UFUNCTION(BlueprintPure)
	int32 GetTier() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ChangeTier(int32 NewTier);

	UFUNCTION(Reliable, NetMulticast)
	void MulticastChangeTier(int32 NewTier);

protected:
	virtual void BeginPlay() override;
	
	void HandleAwaitingUpgradeAnimationOnTick();

	//Synced by reliable upgrade RPC.
	int32 Tier;

	UPROPERTY()
	USkeletalMeshComponent* Skm;

	bool bAwaitingUpgradeAnimation = false;
public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
