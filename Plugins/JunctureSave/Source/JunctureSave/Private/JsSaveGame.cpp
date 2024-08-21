// Fill out your copyright notice in the Description page of Project Settings.


#include "JsSaveGame.h"

#include "JsSaveArchive.h"
#include "JunctureSave.h"

void UJsSaveGame::BindWorld(UWorld* World)
{
	if (BoundWorld)
	{
		UE_LOG(LogJunctureSave, Warning, TEXT("JsSaveGame failed to bind world as a world is already bound."));
		return;
	}
	BoundWorld = World;
	BoundWorld->AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateUObject(this, &UJsSaveGame::OnWorldActorDestroyed));
}

void UJsSaveGame::ReproduceWorld()
{
}

void UJsSaveGame::MarkObjectDirty(UObject* Object, bool bIsMapObject)
{
	//Deal with attachment for actors as well.
}

void UJsSaveGame::RegisterDynamicObject(UObject* DynamicObject)
{
}

void UJsSaveGame::RegisterMapObject(UObject* MapObject)
{
}

void UJsSaveGame::PreloadObject(UWorld* World, FJsObjectRecord& ObjectRecord)
{
	
}

void UJsSaveGame::SaveData(UObject* Object, TArray<uint8>& Data)
{
	if (!Object) return;

	FMemoryWriter MemoryWriter = FMemoryWriter(Data, true);
	FJsSaveArchive Ar = FJsSaveArchive(MemoryWriter);

	Object->Serialize(Ar);
}

void UJsSaveGame::LoadData(UObject* Object, TArray<uint8>& Data)
{
	if (!Object) return;

	FMemoryReader MemoryReader(Data, true);

	FJsSaveArchive Ar(MemoryReader);
	Object->Serialize(Ar);
}

void UJsSaveGame::OnWorldActorDestroyed(AActor* Actor)
{
}
