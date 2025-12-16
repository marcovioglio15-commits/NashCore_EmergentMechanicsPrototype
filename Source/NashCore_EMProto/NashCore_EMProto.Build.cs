// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NashCore_EMProto : ModuleRules
{
	public NashCore_EMProto(ReadOnlyTargetRules Target) : base(Target)
	{
		// Enable explicit or shared precompiled headers to keep compile times predictable.
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Expose core engine and input modules required by the simulation framework.
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "NavigationSystem", "AIModule", "GameplayTasks" });

		// No private-only dependencies are needed at this stage; keep the list explicit for clarity.
		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
