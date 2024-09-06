// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsActorRecord.h"
#include "GameFramework/SaveGame.h"
#include "JsSaveGame.generated.h"

struct FJsActorRecord;

/**
 * Save: MarkActorDirty -> SerializeActors -> (Async)SaveGameToSlot.
 * Load: (Async)LoadGameFromSlot -> DeserializeActors.
 * Use MarkDirty to save or resave an Actor the next time SerializeActors() is called.
 */
UCLASS(Blueprintable)
class JUNCTURESAVE_API UJsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	//Actor records that are written to and read from the disk.
	UPROPERTY()
	TArray<FJsActorRecord> ActorRecords;

	//Tracked non-map actors with their respective record index.
	UPROPERTY(Transient)
	TMap<UObject*, int32> DynamicActors;

	//Tracked map actors with their respective record index.
	UPROPERTY(Transient)
	TMap<UObject*, int32> MapActors;

	//Actors with outdated records.
	UPROPERTY(Transient)
	TSet<UObject*> DirtyActors;

public:
	UFUNCTION(BlueprintCallable)
	void DeserializeActors();

	UFUNCTION(BlueprintCallable)
	void MarkActorDirty(AActor* Actor, bool bIsMapActor);

	UFUNCTION(BlueprintCallable)
	void MarkActorDestroyed(AActor* Actor);

	void RegisterDynamicActor(AActor* DynamicActor);
	
	void RegisterMapActor(AActor* MapActor);

	void SaveActor(AActor* Actor, FJsActorRecord& ActorRecord, const bool bIsMapActor);
	
	//Spawn DynamicActor from save.
	UObject* PreloadActor(UWorld* World, const FJsActorRecord& ActorRecord);

	void LoadActor(AActor* Actor, FJsActorRecord& ActorRecord);

	static void SaveData(AActor* Actor, TArray<uint8>& Data);

	static void LoadData(AActor* Actor, TArray<uint8>& Data);
};
