using UnrealBuildTool;

public class MegaPlantsBoundsCache : ModuleRules
{
    public MegaPlantsBoundsCache(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings" });
        PrivateDependencyModuleNames.AddRange(new[] { "PCG", "RenderCore", "RHI" });
    }
}
