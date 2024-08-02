using UnrealBuildTool;

public class GameCore : ModuleRules
{
    public GameCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "GameplayAbilities",
                "GameplayTasks",
                "GameplayTags",
                "NetCore",
                "EnhancedInput", 
                "Niagara",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
            }
        );
    }
}