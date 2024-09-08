// Fill out your copyright notice in the Description page of Project Settings.


#include "JsSaveGame.h"

#include "JsSavableActorInterface.h"
#include "JsSaveArchive.h"
#include "JunctureSave.h"

void UJsSaveGame::SerializeActors()
{
	for (AActor* Actor : DirtyActors)
	{
		if (MapActors.Contains(Actor))
		{
			SaveActor(Actor, ActorRecords[MapActors[Actor]], true);
		}
		else if (DynamicActors.Contains(Actor))
		{
			SaveActor(Actor, ActorRecords[MapActors[Actor]], false);
		}
	}
	DirtyActors.Empty();
}

void UJsSaveGame::DeserializeActors()
{
	for (int32 i = 0; i < ActorRecords.Num(); i++)
	{
		//Deal with map actor destruction, and don't spawn for destroyed dynamic actor.
		if (ActorRecords[i].bDestroyed)
		{
			if (ActorRecords[i].Self)
			{
				GetWorld()->DestroyActor(ActorRecords[i].Self);
			}
			continue;
		}

		//Recreate dynamic actors, recreate tracking list for map and dynamic actors.
		if (ActorRecords[i].Self)
		{
			MapActors.Add(ActorRecords[i].Self, i);
		}
		else
		{
			AActor* DynamicActor = PreloadDynamicActor(GetWorld(), i);
			DynamicActors.Add(DynamicActor, i);
			ActorRecords[i].Self = DynamicActor;
		}
	}
	
	for (int32 i = 0; i < ActorRecords.Num(); i++)
	{
		LoadActor(ActorRecords[i].Self, i);
	}

	for (const FJsActorRecord& Record : ActorRecords)
	{
		if (Record.bDestroyed) continue;
		IJsSavableActorInterface* SavableActor = Cast<IJsSavableActorInterface>(Record.Self);
		SavableActor->JsPostDeserialize();
	}
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

void UJsSaveGame::MarkActorDestroyed(AActor* Actor)
{
	if (MapActors.Contains(Actor))
	{
		ActorRecords[MapActors[Actor]].bDestroyed = true;
	}
	else if (DynamicActors.Contains(Actor))
	{
		ActorRecords[DynamicActors[Actor]].bDestroyed = true;
	}
}

void UJsSaveGame::RegisterDynamicActor(AActor* DynamicActor)
{
	if (DynamicActors.Contains(DynamicActor)) return;
	//Add a empty version to the record so we can reference owners with the index.
	ActorRecords.Emplace();
	ActorRecords.Last().Self = DynamicActor;
	DynamicActors.Add(DynamicActor, ActorRecords.Num() - 1);
}

void UJsSaveGame::RegisterMapActor(AActor* MapActor)
{
	if (MapActors.Contains(MapActor)) return;
	//Add a empty version to the record so we can reference owners with the index.
	ActorRecords.Emplace();
	ActorRecords.Last().Self = MapActor;
	MapActors.Add(MapActor, ActorRecords.Num() - 1);
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

	if (Actor->GetOuter())
	{
		if (MapActors.Contains(Cast<AActor>(Actor->GetOuter())))
		{
			ActorRecord.Outer = Actor->GetOuter();
		}
		else
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
	else
	{
		ActorRecord.Outer = nullptr;
		ActorRecord.OuterID = -2;
	}

	if (!bIsMapActor)
	{
		//We set self to nullptr if not a map actor to indicate this is the case after deserialization.
		ActorRecord.Self = nullptr;
	}

	ActorRecord.Name = Actor->GetFName();

	SaveData(Actor, ActorRecord.Data);
	
	ActorRecord.Transform = Actor->GetTransform();
}

AActor* UJsSaveGame::PreloadDynamicActor(UWorld* World, const int32 ActorRecordIndex)
{
	if (ActorRecordIndex >= ActorRecords.Num() || ActorRecordIndex < 0) return nullptr;
	FJsActorRecord& ActorRecord = ActorRecords[ActorRecordIndex];
	if (ActorRecord.bDestroyed) return nullptr;
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

void UJsSaveGame::LoadActor(AActor* Actor, const int32 ActorRecordIndex)
{
	if (ActorRecordIndex >= ActorRecords.Num() || ActorRecordIndex < 0) return;
	
	if (!Actor) return;

	FJsActorRecord& ActorRecord = ActorRecords[ActorRecordIndex];

	Actor->SetActorTransform(ActorRecord.Transform);
	
	LoadData(Actor, ActorRecord.Data);
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
