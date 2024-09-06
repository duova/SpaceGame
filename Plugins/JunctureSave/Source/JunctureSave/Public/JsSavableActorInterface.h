// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JsSavableActorInterface.generated.h"

UINTERFACE()
class UJsSavableActorInterface : public UInterface
{
	GENERATED_BODY()
};

class IJsSavableActorInterface
{
	GENERATED_BODY()

public:

	//Both serialize and deserialize.
	virtual bool JsSerialize(FArchive& Ar);

	//Only called post deserialize.
	virtual bool JsPostDeserialize();
};
