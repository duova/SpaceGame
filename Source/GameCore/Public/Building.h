// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateBuilding);
DECLARE_MULTICAST_DELEGATE(FStaticOnUpdateTier);

class UNiagaraComponent;
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
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDescriptor> Cost;

	FTierInfo();
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
	UTexture2D* Icon;

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
	
	void UpdateCostDisplayInventory();

	virtual void OnChangeTier();

	//Synced by reliable upgrade RPC.
	int32 Tier;

	UPROPERTY(EditAnywhere)
	USceneComponent* SceneComponent;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* MeshComponent;

	bool bAwaitingUpgradeAnimation = false;
	
public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	bool IsMaxTier() const;

	UPROPERTY(EditAnywhere)
	FGameplayTag CostDisplayInventory;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateBuilding OnUpdateTier;

	FStaticOnUpdateTier InternalOnUpdateTier;

	UPROPERTY(BlueprintAssignable)
	FOnUpdateBuilding OnUpdateUniqueName;
};
