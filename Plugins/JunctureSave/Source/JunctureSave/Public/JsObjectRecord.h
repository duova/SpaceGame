// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JsObjectRecord.generated.h"

USTRUCT(BlueprintType)
struct FJsObjectRecord
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	UClass* Class;

	//Outer pointer if outer is existing object.
	UPROPERTY(BlueprintReadWrite)
	UObject* Outer;

	//Outer ID if outer is a saved object.
	UPROPERTY(BlueprintReadWrite)
	int32 OuterID;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsActor;

	//Saves which MutableObject to write back into.
	UPROPERTY(BlueprintReadWrite)
	UObject* Self;
	
	UPROPERTY(BlueprintReadWrite)
	FName Name;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> Data;
	
	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	FJsObjectRecord()
	{
		OuterID = 0;
		bIsActor = false;
		Class = nullptr;
		Outer = nullptr;
		Self = nullptr;
	}
};