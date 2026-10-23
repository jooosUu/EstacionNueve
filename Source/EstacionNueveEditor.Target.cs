using UnrealBuildTool;
using System.Collections.Generic;

public class EstacionNueveEditorTarget : TargetRules
{
	public EstacionNueveEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("EstacionNueve");
	}
}
