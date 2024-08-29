// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Object.h"

struct JUNCTURESAVE_API FJsSaveArchive : public FObjectAndNameAsStringProxyArchive
{
	explicit FJsSaveArchive(FArchive& InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};
