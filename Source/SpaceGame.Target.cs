// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceGameTarget : TargetRules
{
	public SpaceGameTarget(TargetInfo Target) : base(Target)
	{
		//BuildEnvironment = TargetBuildEnvironment.Unique;
		//bOverrideBuildEnvironment = true;
		bWithPushModel = true;
		bUsesSteam = true;
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("SpaceGame");
		RegisterModulesCreatedByRider();
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[] { "GameCore", "Buildings" });
	}
}
