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
	AActor* Self;
	
	UPROPERTY(BlueprintReadWrite)
	FName Name;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> Data;
	
	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite)
	bool bDestroyed;

	FJsActorRecord()
	{
		//-2 is for MapActors.
		OuterID = -2;
		Class = nullptr;
		Outer = nullptr;
		Self = nullptr;
		bDestroyed = false;
	}

	bool Serialize(FArchive& Ar)
	{
		Ar << bDestroyed;
		Ar << Self;
		if (!bDestroyed)
		{
			Ar << Class;
			Ar << Outer;
			Ar << OuterID;
			Ar << Name;
			Ar << Data;
			Ar << Transform;
		}
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FJsActorRecord> : public TStructOpsTypeTraitsBase2<FJsActorRecord>
{
	enum
	{
		WithSerializer = true
	};
};