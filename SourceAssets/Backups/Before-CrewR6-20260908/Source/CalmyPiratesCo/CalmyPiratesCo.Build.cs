using UnrealBuildTool;

public class CalmyPiratesCo : ModuleRules
{
	public CalmyPiratesCo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
            "AIModule",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralMeshComponent", "Json" });
	}
}
