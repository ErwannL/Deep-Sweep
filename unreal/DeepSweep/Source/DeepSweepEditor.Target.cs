using UnrealBuildTool;
public class DeepSweepEditorTarget : TargetRules
{
	public DeepSweepEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("DeepSweep");
	}
}
