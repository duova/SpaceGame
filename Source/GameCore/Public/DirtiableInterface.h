// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DirtiableInterface.generated.h"

UINTERFACE()
class GAMECORE_API UDirtiableInterface : public UInterface
{
	GENERATED_BODY()
};

class GAMECORE_API IDirtiableInterface
{
	GENERATED_BODY()
public:
	
	virtual void MarkDirty();
};
