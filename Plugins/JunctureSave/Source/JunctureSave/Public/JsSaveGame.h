// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JsSaveGame.generated.h"

struct FJsObjectRecord;

/**
 * Save: BindWorld(before any world changes) -> MarkObjectDirty -> (Async)SaveGameToSlot.
 * Load: BindWorld(before any world changes) -> (Async)LoadGameFromSlot -> ReproduceWorld.
 * Use MarkObjectDirty to add an object to the save game.
 * Ensure references to objects are marked with SaveGame for GC.
 */
UCLASS(Blueprintable)
class JUNCTURESAVE_API UJsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	//Object records that are written to and read from the disk.
	UPROPERTY()
	TArray<FJsObjectRecord> ObjectRecords;

	//Tracked non-map objects with their respective record index.
	UPROPERTY(Transient)
	TMap<UObject*, int32> DynamicObjects;

	//Tracked map objects with their respective record index.
	UPROPERTY(Transient)
	TMap<UObject*, int32> MapObjects;

	//Objects with outdated records.
	UPROPERTY(Transient)
	TSet<UObject*> DirtyObjects;

	//Used to track actor destroys with listener or specify which world to reproduce to.
	UPROPERTY()
	UWorld* BoundWorld;

	//Used to see which actor destroys to actually save and which ones to just remove from dynamic objects.
	UPROPERTY()
	TSet<AActor*> MapActors;

public:
	UFUNCTION(BlueprintCallable)
	void BindWorld(UWorld* World);
	
	UFUNCTION(BlueprintCallable)
	void ReproduceWorld();

	UFUNCTION(BlueprintCallable)
	void MarkObjectDirty(UObject* Object, bool bIsMapObject);
	
	void RegisterDynamicObject(UObject* DynamicObject);
	
	void RegisterMapObject(UObject* MapObject);

	//Spawn AddedObject from save.
	void PreloadObject(UWorld* World, FJsObjectRecord& ObjectRecord);

	static void SaveData(UObject* Object, TArray<uint8>& Data);

	static void LoadData(UObject* Object, TArray<uint8>& Data);

	void OnWorldActorDestroyed(AActor* Actor);
};
