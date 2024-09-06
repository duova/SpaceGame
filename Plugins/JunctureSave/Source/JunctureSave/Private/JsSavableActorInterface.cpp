// Fill out your copyright notice in the Description page of Project Settings.


#include "JsSavableActorInterface.h"

bool IJsSavableActorInterface::JsSerialize(FArchive& Ar)
{
	return false;
}

bool IJsSavableActorInterface::JsPostDeserialize()
{
	return false;
}
