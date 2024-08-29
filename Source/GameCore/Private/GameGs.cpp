// Fill out your copyright notice in the Description page of Project Settings.


#include "GameGs.h"

#include "Building.h"
#include "GameCore.h"
#include "Item.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

bool FUniquelyNamedBuilding::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	UObject* BuildingPtr = Building;
	Map->SerializeObject(Ar, ABuilding::StaticClass(), BuildingPtr);
	if (!Ar.IsSaving())
	{
		Building = Cast<ABuilding>(BuildingPtr);
	}
	Ar << Name;
	return bOutSuccess;
}

FUniquelyNamedBuilding::FUniquelyNamedBuilding(): Building(nullptr)
{
}

FUniquelyNamedBuilding::FUniquelyNamedBuilding(ABuilding* InBuilding, const FString& InName)
{
	Building = InBuilding;
	Name = InName;
}

FItemDescriptor::FItemDescriptor(): ItemCount(0)
{
}

FItemDescriptor::FItemDescriptor(const TSubclassOf<UItem>& InItemClass, const int32 InItemCount): ItemCount(InItemCount)
{
	ItemClass = InItemClass;
}

bool FRecipe::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar.SerializeBits(&bUnlocked, 1);
	return bOutSuccess;
}

FRecipe::FRecipe(): Icon(nullptr), RecipeBaseTime(0), ResearchBaseTime(0), bUnlocked(false)
{
}

AGameGs::AGameGs()
{
	SetReplicates(true);
}

void AGameGs::RegisterItemClasses()
{
	// Get the pointer
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// For a scan to make sure AssetRegistry is ready
	TArray<FString> ContentPaths;
	// This will scan the full folder, but obviously it's preferable to work on a dedicated one such as "/Game/Blueprints":
	ContentPaths.Add(ItemSearchPath);
	AssetRegistry.ScanPathsSynchronous(ContentPaths);

	TSet<FTopLevelAssetPath> DerivedClassNames;
	TArray<FTopLevelAssetPath> ClassNames{UItem::StaticClass()->GetClassPathName()};
	TSet<FTopLevelAssetPath> ExcludedClassNames;
	AssetRegistry.GetDerivedClassNames(ClassNames, ExcludedClassNames, DerivedClassNames);

	TArray<FAssetData> BlueprintAssets;
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(FTopLevelAssetPath(ItemSearchPath)); // If you want to limit the search to a specific folder
	Filter.bRecursivePaths = true;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	for (FAssetData const& BlueprintAsset : BlueprintAssets)
	{
		const FAssetTagValueRef GeneratedClassTag = BlueprintAsset.TagsAndValues.FindTag(FName("GeneratedClass"));
		if (GeneratedClassTag.IsSet())
		{
			FTopLevelAssetPath ClassPath(GeneratedClassTag.GetValue());
			if (DerivedClassNames.Contains(ClassPath))
			{
				// Now we can retrieve the class.
				FStringBuilderBase Builder;
				Builder << BlueprintAsset.PackageName << '.' << BlueprintAsset.AssetName << "_C";
				UClass* BlueprintGeneratedClass = LoadObject<UClass>(nullptr, Builder.ToString());
				if (TSubclassOf<UItem> ItemClass = BlueprintGeneratedClass)
				{
					if (ItemClass.GetDefaultObject()->bDoNotRegister) continue;
					ItemRegistry.Add(ItemClass);
				}
			}
		}
	}
}

void AGameGs::BeginPlay()
{
	Super::BeginPlay();

	RegisterItemClasses();
}

void AGameGs::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AGameGs, Recipes, RepParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AGameGs, UniquelyNamedBuildings, RepParams);
}

void AGameGs::MarkRecipesDirty() const
{
	MARK_PROPERTY_DIRTY_FROM_NAME(AGameGs, Recipes, this);
}

void AGameGs::OnRep_Recipes() const
{
	if (!OnRecipeUnlocksUpdated.IsBound()) return;
	OnRecipeUnlocksUpdated.Broadcast();
}

bool AGameGs::AddUniquelyNamedBuilding(const FString& Name, ABuilding* Building)
{
	if (!HasAuthority()) return false;
	if (!Building) return false;
	if (!UniqueBuildingNameAvailable(Name)) return false;
	//Remove previous name if it exists.
	RemoveUniquelyNamedBuilding(Building);
	UniquelyNamedBuildings.Emplace(Building, Name);
	MARK_PROPERTY_DIRTY_FROM_NAME(AGameGs, UniquelyNamedBuildings, this);
	OnRep_UniquelyNamedBuildings();
	return true;
}

bool AGameGs::RemoveUniquelyNamedBuilding(ABuilding* Building)
{
	if (!HasAuthority()) return false;
	if (!Building) return false;
	for (int32 i = UniquelyNamedBuildings.Num() - 1; i >= 0; i--)
	{
		if (Building != UniquelyNamedBuildings[i].Building) continue;
		UniquelyNamedBuildings.RemoveAt(i);
		MARK_PROPERTY_DIRTY_FROM_NAME(AGameGs, UniquelyNamedBuildings, this);
		OnRep_UniquelyNamedBuildings();
		return true;
	}
	return false;
}

bool AGameGs::UniqueBuildingNameAvailable(const FString& Name)
{
	if (Name.IsEmpty()) return false;
	for (const FUniquelyNamedBuilding& NamedBuilding : UniquelyNamedBuildings)
	{
		if (NamedBuilding.Name.Equals(Name, ESearchCase::IgnoreCase)) return false;
	}
	return true;
}

ABuilding* AGameGs::GetBuildingByUniqueName(const FString& Name)
{
	for (const FUniquelyNamedBuilding& NamedBuilding : UniquelyNamedBuildings)
	{
		if (NamedBuilding.Name == Name) return NamedBuilding.Building;
	}
	return nullptr;
}

FString AGameGs::GetUniqueNameByBuilding(const ABuilding* Building)
{
	if (!Building) return FString();
	for (const FUniquelyNamedBuilding& NamedBuilding : UniquelyNamedBuildings)
	{
		if (NamedBuilding.Building == Building) return NamedBuilding.Name;
	}
	return FString();
}

TArray<FString> AGameGs::GetAllUniqueBuildingNames()
{
	TArray<FString> RetVal;
	for (const FUniquelyNamedBuilding& NamedBuilding : UniquelyNamedBuildings)
	{
		RetVal.Add(NamedBuilding.Name);
	}
	return RetVal;
}

void AGameGs::OnRep_UniquelyNamedBuildings()
{
	for (const FUniquelyNamedBuilding& NamedBuilding : UniquelyNamedBuildings)
	{
		if (NamedBuilding.Building->OnUpdateUniqueName.IsBound())
		{
			NamedBuilding.Building->OnUpdateUniqueName.Broadcast();
		}
	}
}
