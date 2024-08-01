// Fill out your copyright notice in the Description page of Project Settings.


#include "GameGs.h"

#include "GameCore.h"
#include "Item.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

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
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
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
