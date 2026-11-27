using UnrealBuildTool;
public class DeepSweep : ModuleRules
{
	public DeepSweep(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"Niagara", "PhysicsCore", "NetCore", "UMG", "Slate", "SlateCore",
			"OnlineSubsystem", "OnlineSubsystemUtils"
		});
	}
}
