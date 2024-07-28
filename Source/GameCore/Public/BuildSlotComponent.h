// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BuildSlotComponent.generated.h"


class ABuilding;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class GAMECORE_API UBuildSlotComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBuildSlotComponent();

	UFUNCTION(NetMulticast, Reliable)
	void Enable();

	UFUNCTION(NetMulticast, Reliable)
	void Disable();

	UPROPERTY(Replicated)
	ABuilding* CurrentBuilding;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ABuilding> DefaultBuilding;

	bool bEnabled;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ChangeBuilding(const TSubclassOf<ABuilding>& BuildingClass);

	UPROPERTY(EditAnywhere)
	int32 UnlockTier = 0;
};
