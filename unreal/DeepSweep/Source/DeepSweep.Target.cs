using UnrealBuildTool;
public class DeepSweepTarget : TargetRules
{
	public DeepSweepTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("DeepSweep");
	}
}
