// Fill out your copyright notice in the Description page of Project Settings.


#include "JsSaveGame.h"

#include "JsSavableActorInterface.h"
#include "JsSaveArchive.h"
#include "JunctureSave.h"

void UJsSaveGame::DeserializeActors()
{
	//Do owner first after preloading.
	//Then make sure we link all components to actors and do attachments simultaneously.
	//Then LoadObject.
}

void UJsSaveGame::MarkActorDirty(AActor* Actor, const bool bIsMapActor)
{
	if (bIsMapActor)
	{
		RegisterMapActor(Actor);
	}
	else
	{
		RegisterDynamicActor(Actor);
	}
	DirtyActors.Add(Actor);
}

void UJsSaveGame::RegisterDynamicActor(AActor* DynamicActor)
{
	if (DynamicActors.Contains(DynamicActor)) return;
	DynamicActors.Add(DynamicActor);
	//Add a empty version to the record so we can reference owners with the index.
	ActorRecords.Emplace();
	ActorRecords.Last().Self = DynamicActor;
}

void UJsSaveGame::RegisterMapActor(AActor* MapActor)
{
	if (MapActors.Contains(MapActor)) return;
	MapActors.Add(MapActor);
	//Add a empty version to the record so we can reference owners with the index.
	ActorRecords.Emplace();
	ActorRecords.Last().Self = MapActor;
}

void UJsSaveGame::SaveActor(AActor* Actor, FJsActorRecord& ActorRecord, const bool bIsMapActor)
{
	if (!Actor)
	{
		UE_LOG(LogJunctureSave, Warning, TEXT("Invalid Save Object!"))
		return;
	}

	if (Actor->HasAnyFlags(EObjectFlags::RF_Transient))
	{
		UE_LOG(LogJunctureSave, Warning, TEXT("Tried to save RF_Transient object"))
		return;
	}

	ActorRecord.Class = Actor->GetClass();

	if (bIsMapActor)
	{
		ActorRecord.Outer = Actor->GetOuter();
	}
	else
	{
		if (Actor->GetOuter())
		{
			for (uint8 i = 0; i < ActorRecords.Num(); i++)
			{
				if (ActorRecords[i].Self == Actor->GetOuter())
				{
					ActorRecord.OuterID = i;
					break;
				}
			}
		}
	}

	ActorRecord.Name = Actor->GetFName();

	SaveData(Actor, ActorRecord.Data);
}

UObject* UJsSaveGame::PreloadActor(UWorld* World, const FJsActorRecord& ActorRecord)
{
	UObject* Outer;
	if (ActorRecord.Outer)
	{
		Outer = ActorRecord.Outer;
	}
	else
	{
		//Object records are in creation order so outers should already exist for dynamic objects.
		if (ActorRecords.Num() <= ActorRecord.OuterID)
		{
			UE_LOG(LogJunctureSave, Warning, TEXT("Saved object failed to preload due to invalid outer ID."));
			return nullptr;
		}
		if (ActorRecord.OuterID < 0) Outer = nullptr;
		Outer = ActorRecords[ActorRecord.OuterID].Self;
	}
	FActorSpawnParameters ActorParams;
	ActorParams.Name = ActorRecord.Name;
	AActor* Actor = World->SpawnActor(ActorRecord.Class, nullptr, nullptr, ActorParams);

	if (Outer && Actor)
	{
		Actor->Rename(nullptr, Outer);
	}

	return Actor;
}

void UJsSaveGame::LoadActor(AActor* Actor, FJsActorRecord& ActorRecord)
{

}

void UJsSaveGame::SaveData(AActor* Actor, TArray<uint8>& Data)
{
	if (!Actor) return;

	IJsSavableActorInterface* SavableActor = Cast<IJsSavableActorInterface>(Actor);
	
	if (!SavableActor) return;
	
	FMemoryWriter MemoryWriter = FMemoryWriter(Data, true);
	FJsSaveArchive Ar = FJsSaveArchive(MemoryWriter);
	Ar.SetIsSaving(true);
	Ar.SetIsLoading(false);
	
	SavableActor->JsSerialize(Ar);
}

void UJsSaveGame::LoadData(AActor* Actor, TArray<uint8>& Data)
{
	if (!Actor) return;

	IJsSavableActorInterface* SavableActor = Cast<IJsSavableActorInterface>(Actor);
	
	if (!SavableActor) return;
	
	FMemoryReader MemoryReader(Data, true);
	FJsSaveArchive Ar(MemoryReader);
	Ar.SetIsSaving(false);
	Ar.SetIsLoading(true);
	
	SavableActor->JsSerialize(Ar);
}
