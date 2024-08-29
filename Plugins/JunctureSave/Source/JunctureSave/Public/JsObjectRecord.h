// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JsObjectRecord.generated.h"

UENUM(BlueprintType)
enum class EObjectType : uint8
{
	Object,
	ActorComponent,
	SceneComponent,
	Actor
};

USTRUCT(BlueprintType)
struct FJsObjectRecord
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
	EObjectType ObjectType;
	
	UPROPERTY(BlueprintReadWrite)
	UObject* Self;
	
	UPROPERTY(BlueprintReadWrite)
	FName Name;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> Data;
	
	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	//If attached parent is a map object.
	UPROPERTY(BlueprintReadWrite)
	USceneComponent* AttachParent;

	//If attached parent is a dynamic object.
	UPROPERTY(BlueprintReadWrite)
	int32 AttachParentID;
	
	UPROPERTY(BlueprintReadWrite)
	FName SocketName;
	
	UPROPERTY(BlueprintReadWrite)
	bool bKeepWorldLocation;

	UPROPERTY(BlueprintReadWrite)
	bool bKeepWorldRotation;

	UPROPERTY(BlueprintReadWrite)
	bool bKeepWorldScale;

	UPROPERTY(BlueprintReadWrite)
	bool bWeldSimulatedBodies;

	FJsObjectRecord()
	{
		//-2 is for MapObjects.
		OuterID = -2;
		ObjectType = EObjectType::Object;
		Class = nullptr;
		Outer = nullptr;
		Self = nullptr;
		AttachParent = nullptr;
		AttachParentID = -2;
		bKeepWorldLocation = false;
		bKeepWorldRotation = false;
		bKeepWorldScale = false;
	}
};