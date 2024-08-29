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
	//Do owner first after preloading.
	//Then make sure we link all components to actors and do attachments simultaneously.
	//Then LoadObject.
}

void UJsSaveGame::MarkObjectDirty(UObject* Object, const bool bIsMapObject)
{
	if (bIsMapObject)
	{
		RegisterMapObject(Object);
	}
	else
	{
		RegisterDynamicObject(Object);
	}
	DirtyObjects.Add(Object);
}

void UJsSaveGame::RegisterDynamicObject(UObject* DynamicObject)
{
	if (DynamicObjects.Contains(DynamicObject)) return;
	DynamicObjects.Add(DynamicObject);
	//Add a empty version to the record so we can reference owners with the index.
	ObjectRecords.Emplace();
	ObjectRecords.Last().Self = DynamicObject;
}

void UJsSaveGame::RegisterMapObject(UObject* MapObject)
{
	if (MapObjects.Contains(MapObject)) return;
	MapObjects.Add(MapObject);
	//Add a empty version to the record so we can reference owners with the index.
	ObjectRecords.Emplace();
	ObjectRecords.Last().Self = MapObject;
}

void UJsSaveGame::SaveObject(UObject* Object, FJsObjectRecord& ObjectRecord, const bool bIsMapObject)
{
	if (!Object)
	{
		UE_LOG(LogJunctureSave, Warning, TEXT("Invalid Save Object!"))
		return;
	}

	if (Object->HasAnyFlags(EObjectFlags::RF_Transient))
	{
		UE_LOG(LogJunctureSave, Warning, TEXT("Tried to save RF_Transient object"))
		return;
	}
	
	ObjectRecord.Class = Object->GetClass();
	
	if (bIsMapObject)
	{
		ObjectRecord.Outer = Object->GetOuter();
	}
	else
	{
		if (Object->GetOuter())
		{
			for (uint8 i = 0; i < ObjectRecords.Num(); i++)
			{
				if (ObjectRecords[i].Self == Object->GetOuter())
				{
					ObjectRecord.OuterID = i;
					break;
				}
			}
		}
	}

	if (Object->IsA(AActor::StaticClass()))
	{
		ObjectRecord.ObjectType = EObjectType::Actor;
	}
	else if (Object->IsA(USceneComponent::StaticClass()))
	{
		ObjectRecord.ObjectType = EObjectType::SceneComponent;
	}
	else if (Object->IsA(UActorComponent::StaticClass()))
	{
		ObjectRecord.ObjectType = EObjectType::ActorComponent;
	}
	else
	{
		ObjectRecord.ObjectType = EObjectType::Object;
	}

	ObjectRecord.Name = Object->GetFName();

	SaveData(Object, ObjectRecord.Data);

	USceneComponent* SceneComponent = Cast<USceneComponent>(Object);

	if (SceneComponent)
	{
		ObjectRecord.Transform = SceneComponent->GetComponentTransform();
		ObjectRecord.AttachParent = SceneComponent->GetAttachParent();
		if (ObjectRecord.AttachParent)
		{
			ObjectRecord.SocketName = SceneComponent->GetAttachSocketName();
			ObjectRecord.bKeepWorldLocation = SceneComponent->IsUsingAbsoluteLocation();
			ObjectRecord.bKeepWorldRotation = SceneComponent->IsUsingAbsoluteRotation();
			ObjectRecord.bKeepWorldScale = SceneComponent->IsUsingAbsoluteScale();
		}
	}
}

UObject* UJsSaveGame::PreloadObject(UWorld* World, FJsObjectRecord& ObjectRecord)
{
	UObject* Outer = nullptr;
	if (ObjectRecord.Outer)
	{
		Outer = ObjectRecord.Outer;
	}
	else
	{
		//Object records are in creation order so outers should already exist for dynamic objects.
		if (ObjectRecords.Num() <= ObjectRecord.OuterID)
		{
			UE_LOG(LogJunctureSave, Warning, TEXT("Saved object failed to preload due to invalid outer ID."));
			return nullptr;
		}
		if (ObjectRecord.OuterID < 0) Outer = nullptr;
		Outer = ObjectRecords[ObjectRecord.OuterID].Self;
	}
	UObject* Object = nullptr;
	if (ObjectRecord.ObjectType == EObjectType::Actor)
	{
		FActorSpawnParameters ActorParams;
		ActorParams.Name = ObjectRecord.Name;
		Object = World->SpawnActor(ObjectRecord.Class, nullptr, nullptr, ActorParams);
	}
	else
	{
		Object = NewObject<UObject>(Outer, ObjectRecord.Class, ObjectRecord.Name);
	}
	//Make sure we set self for the object record so we can reference for later outers.
	//Duplicated components from spawn actor?
	//TODO: how to delta save component add & remove in actor, track which components to replace, delete, and add.
	//Index components by CDO order at the start somehow to keep track of component changes?
	//Using world hooks to update the index from added or removed components, and save the updated indexes to the actor object.

	return nullptr;
}

void UJsSaveGame::LoadObject(UObject* Object, FJsObjectRecord& ObjectRecord)
{
	//Go through ownership chain and set component actor properly.
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
