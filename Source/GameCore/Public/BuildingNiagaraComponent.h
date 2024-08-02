// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "BuildingNiagaraComponent.generated.h"


class ABuilding;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMECORE_API UBuildingNiagaraComponent : public UNiagaraComponent
{
	GENERATED_BODY()

public:
	UBuildingNiagaraComponent();
	
	UPROPERTY(EditAnywhere)
	TArray<int32> ActiveOnTiers;

	UPROPERTY()
	ABuilding* Building;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnUpdateTier();
};
