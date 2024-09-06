// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsActorRecord.generated.h"

USTRUCT(BlueprintType)
struct FJsActorRecord
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	UClass* Class;

	//Outer pointer if outer is map object.
	UPROPERTY(BlueprintReadWrite)
	UObject* Outer;

	//Outer ID if outer is a dynamic object.
	UPROPERTY(BlueprintReadWrite)
	int32 OuterID;
	
	UPROPERTY(BlueprintReadWrite)
	UObject* Self;
	
	UPROPERTY(BlueprintReadWrite)
	FName Name;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> Data;
	
	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	FJsActorRecord()
	{
		//-2 is for MapActors.
		OuterID = -2;
		Class = nullptr;
		Outer = nullptr;
		Self = nullptr;
	}
};