// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceGameTarget : TargetRules
{
	public SpaceGameTarget(TargetInfo Target) : base(Target)
	{
		//BuildEnvironment = TargetBuildEnvironment.Unique;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		Type = TargetType.Game;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		//bOverrideBuildEnvironment = true;
		//bWithPushModel = true;
		bUsesSteam = true;
		ExtraModuleNames.Add("SpaceGame");
		RegisterModulesCreatedByRider();
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[] { "GameCore", "Buildings" });
	}
}
