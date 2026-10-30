#include "MegaPlantsBoundsCacheHelper.h"

#include "MegaPlantsBoundsCacheSettings.h"

#include "Animation/TransformProviderData.h"
// UE 5.7 places UE_EXPERIMENTAL before this class declaration, which MSVC
// rejects when experimental API validation expands it to [[deprecated]].
// Suppress only that annotation while parsing this engine header.
#pragma push_macro("UE_EXPERIMENTAL")
#undef UE_EXPERIMENTAL
#define UE_EXPERIMENTAL(Version, Message)
#include "Components/InstancedSkinnedMeshComponent.h"
#pragma pop_macro("UE_EXPERIMENTAL")
#include "Engine/SkinnedAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PCGComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/UObjectIterator.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMegaPlantsBoundsCache, Log, All);

namespace
{
    TAutoConsoleVariable<int32> CVarFoliageBoundsCache(
        TEXT("MegaPlants.BoundsCache"), -1,
        TEXT("-1: follow Project Settings; 0: disable plugin bounds cache; 1: enable. Applies before the next world tick. Does not clear other systems' overrides."));

    FAutoConsoleCommandWithWorld FoliageBoundsStatusCommand(
        TEXT("MegaPlants.BoundsCache.Status"),
        TEXT("Report actual instanced skinned mesh bounds overrides in this game/PIE world."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (!World || !World->IsGameWorld()) return;
            for (TObjectIterator<UMegaPlantsBoundsCacheHelper> It; It; ++It)
                if (!It->IsTemplate() && It->GetWorld() == World) It->LogStatus();
        }));
}

UWorld* UMegaPlantsBoundsCacheHelper::GetWorld() const
{
    return TargetWorld.Get();
}

void UMegaPlantsBoundsCacheHelper::Start(UWorld* World, float PaddingCm)
{
    TargetWorld = World;
    Padding = FMath::Max(0.f, PaddingCm);
    BeforeTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &ThisClass::BeforeWorldTick);
    UpdateEnabledState();
    LogStatus();
}

void UMegaPlantsBoundsCacheHelper::UpdateEnabledState()
{
    const UMegaPlantsBoundsCacheSettings* Settings = UMegaPlantsBoundsCacheSettings::Get();
    const int32 Override = CVarFoliageBoundsCache.GetValueOnGameThread();
    const bool bRequested = Override < 0 ? Settings->bCacheFoliageBounds : Override != 0;
    const float RequestedPadding = FMath::Max(0.f, Settings->FoliageBoundsPaddingCm);
    if (bLoggedInitialState && bEnabled == bRequested && Padding == RequestedPadding) return;
    const int32 PreviousCount = CachedComponents.Num();
    Invalidate();
    bEnabled = bRequested;
    Padding = RequestedPadding;
    bLoggedInitialState = true;
    UE_LOG(LogMegaPlantsBoundsCache, Log, TEXT("Foliage bounds cache %s: ProjectSetting=%s, ConsoleOverride=%d, Padding=%.0f cm, PreviousCached=%d, CurrentCached=%d. World=%s"),
        bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"), Settings->bCacheFoliageBounds ? TEXT("On") : TEXT("Off"),
        Override, Padding, PreviousCount, CachedComponents.Num(), *GetNameSafe(TargetWorld.Get()));
}

void UMegaPlantsBoundsCacheHelper::LogStatus() const
{
    int32 Total = 0, Eligible = 0, WithOverride = 0;
    for (TObjectIterator<UInstancedSkinnedMeshComponent> It; It; ++It)
    {
        UInstancedSkinnedMeshComponent* Component = *It;
        if (!IsValid(Component) || Component->GetWorld() != TargetWorld.Get() || !Component->IsRegistered()) continue;
        ++Total;
        const bool bEligible = CanCache(Component);
        const bool bHasOverride = Component->GetPrimitiveBoundsOverride().IsValid != 0;
        Eligible += bEligible ? 1 : 0;
        WithOverride += bHasOverride ? 1 : 0;
        UE_LOG(LogMegaPlantsBoundsCache, Log, TEXT("Foliage bounds component %s: Instances=%d, Eligible=%d, Override=%s, TickEnabled=%d, Provider=%s"),
            *Component->GetPathName(), Component->GetInstanceCount(), bEligible,
            bHasOverride ? (CachedComponents.Contains(Component) ? TEXT("Plugin") : TEXT("External/Pre-existing")) : TEXT("None"),
            Component->IsComponentTickEnabled(), *GetNameSafe(Component->GetTransformProvider()));
    }
    UE_LOG(LogMegaPlantsBoundsCache, Log, TEXT("Foliage bounds status: Enabled=%d, Components=%d, Eligible=%d, PluginCached=%d, TotalOverrides=%d, ConsoleOverride=%d, ProjectSetting=%d, World=%s"),
        bEnabled, Total, Eligible, CachedComponents.Num(), WithOverride, CVarFoliageBoundsCache.GetValueOnGameThread(),
        UMegaPlantsBoundsCacheSettings::Get()->bCacheFoliageBounds, *GetNameSafe(TargetWorld.Get()));
}

bool UMegaPlantsBoundsCacheHelper::CanCache(UInstancedSkinnedMeshComponent* Component) const
{
    static const FName SkipTag(TEXT("SkipMegaPlantsBoundsCache"));
    if (!IsValid(Component) || Component->GetWorld() != TargetWorld.Get() || !Component->IsRegistered()
        || Component->UsesGPUOnlyInstances() || Component->GetInstanceCount() == 0
        || Component->ComponentHasTag(SkipTag)
        || (Component->GetOwner() && Component->GetOwner()->ActorHasTag(SkipTag))) return false;

    const USkinnedAsset* Asset = Component->GetSkinnedAsset();
    const UTransformProviderData* Provider = Component->GetTransformProvider();
    if (!Asset || Asset->IsCompiling() || !Provider || Provider->IsCompiling()
        || !Provider->IsEnabled()) return false;

    // No hard dependency or asset load: Dynamic Wind remains an optional engine plugin.
    for (const UClass* Class = Provider->GetClass(); Class; Class = Class->GetSuperClass())
    {
        if (Class->GetPathName() == TEXT("/Script/DynamicWind.DynamicWindData")) return true;
    }
    return false;
}

void UMegaPlantsBoundsCacheHelper::DiscoverComponents()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(MegaPlants_BoundsCache_Discover);
    for (TObjectIterator<UPCGComponent> It; It; ++It)
    {
        UPCGComponent* Component = *It;
        if (!IsValid(Component) || Component->GetWorld() != TargetWorld.Get() || PCGComponents.Contains(Component)) continue;
        PCGComponents.Add(Component);
        Component->OnPCGGraphStartGeneratingDelegate.AddUObject(this, &ThisClass::OnPCGChanged);
        Component->OnPCGGraphGeneratedDelegate.AddUObject(this, &ThisClass::OnPCGChanged);
        Component->OnPCGGraphCancelledDelegate.AddUObject(this, &ThisClass::OnPCGChanged);
        Component->OnPCGGraphCleanedDelegate.AddUObject(this, &ThisClass::OnPCGChanged);
    }
    for (TObjectIterator<UInstancedSkinnedMeshComponent> It; It; ++It)
    {
        if (IsValid(*It) && It->GetWorld() == TargetWorld.Get()) Candidates.Add(*It);
    }
    DiscoveryCountdown = 1.f;
}

void UMegaPlantsBoundsCacheHelper::Restore(UInstancedSkinnedMeshComponent* Component, const FCachedBounds& Cached)
{
    // Never overwrite a replacement override installed by another system.
    if (IsValid(Component) && Component->GetPrimitiveBoundsOverride() == Cached.AppliedBounds)
    {
        Component->SetPrimitiveBoundsOverride(FBox(ForceInit));
        Component->UpdateBounds();
        Component->MarkRenderTransformDirty();
    }
}

void UMegaPlantsBoundsCacheHelper::Invalidate()
{
    for (const auto& Entry : CachedComponents) Restore(Entry.Key.Get(), Entry.Value);
    CachedComponents.Empty();
    DiscoveryCountdown = 0.f;
}

void UMegaPlantsBoundsCacheHelper::OnPCGChanged(UPCGComponent* Component)
{
    // Graphs may target actors other than their owner, including partition actors.
    // Invalidating all owned overrides avoids guessing which components they will edit.
    Invalidate();
}

void UMegaPlantsBoundsCacheHelper::BeforeWorldTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
    if (World != TargetWorld.Get()) return;
    UpdateEnabledState();
    if (!bEnabled) return;
    TRACE_CPUPROFILER_EVENT_SCOPE(MegaPlants_BoundsCache_Update);
    DiscoveryCountdown -= DeltaSeconds;
    if (DiscoveryCountdown <= 0.f) DiscoverComponents();

    bool bGenerating = false;
    for (auto It = PCGComponents.CreateIterator(); It; ++It)
    {
        if (UPCGComponent* Component = It->Get())
            bGenerating |= Component->IsGenerating() || Component->IsCleaningUp();
        else It.RemoveCurrent();
    }
    if (bGenerating)
    {
        if (!CachedComponents.IsEmpty()) Invalidate();
        return;
    }

    for (auto It = CachedComponents.CreateIterator(); It; ++It)
    {
        UInstancedSkinnedMeshComponent* Component = It.Key().Get();
        const FCachedBounds& Cached = It.Value();
        if (!CanCache(Component) || Component->GetPrimitiveBoundsOverride() != Cached.AppliedBounds
            || Component->GetInstanceCount() != Cached.InstanceCount
            || Component->GetInstanceData().GetData() != Cached.InstanceStorage
            || !Component->GetComponentTransform().Equals(Cached.Transform)
            || Component->GetSkinnedAsset() != Cached.Asset.Get()
            || Component->GetTransformProvider() != Cached.Provider.Get())
        {
            Restore(Component, Cached);
            It.RemoveCurrent();
        }
    }

    int32 AppliedCount = 0;
    for (auto It = Candidates.CreateIterator(); It; ++It)
    {
        UInstancedSkinnedMeshComponent* Component = It->Get();
        if (!IsValid(Component)) { It.RemoveCurrent(); continue; }
        if (CachedComponents.Contains(Component) || !CanCache(Component)
            || Component->GetPrimitiveBoundsOverride().IsValid) continue;

        TRACE_CPUPROFILER_EVENT_SCOPE(MegaPlants_BoundsCache_Build);
        // CalcBounds aggregates all instances. Dynamic Wind currently uses mesh asset
        // bounds (it supplies no animation bounds); padding covers extra wind motion.
        // The primitive override is world-space.
        const FBox Bounds = Component->CalcBounds(Component->GetComponentTransform()).GetBox().ExpandBy(Padding);
        if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN()) continue;
        FCachedBounds Cached;
        Cached.AppliedBounds = Bounds;
        Cached.Transform = Component->GetComponentTransform();
        Cached.InstanceCount = Component->GetInstanceCount();
        Cached.InstanceStorage = Component->GetInstanceData().GetData();
        Cached.Asset = Component->GetSkinnedAsset();
        Cached.Provider = Component->GetTransformProvider();
        Component->SetPrimitiveBoundsOverride(Bounds);
        Component->UpdateBounds();
        Component->MarkRenderTransformDirty();
        CachedComponents.Add(Component, MoveTemp(Cached));
        ++AppliedCount;
    }
    if (AppliedCount > 0)
        UE_LOG(LogMegaPlantsBoundsCache, Log, TEXT("Cached foliage bounds for %d components (%d total)."), AppliedCount, CachedComponents.Num());
}

void UMegaPlantsBoundsCacheHelper::Shutdown()
{
    FWorldDelegates::OnWorldPreActorTick.Remove(BeforeTickHandle);
    for (const auto& WeakComponent : PCGComponents)
    {
        if (UPCGComponent* Component = WeakComponent.Get())
        {
            Component->OnPCGGraphStartGeneratingDelegate.RemoveAll(this);
            Component->OnPCGGraphGeneratedDelegate.RemoveAll(this);
            Component->OnPCGGraphCancelledDelegate.RemoveAll(this);
            Component->OnPCGGraphCleanedDelegate.RemoveAll(this);
        }
    }
    Invalidate();
    Candidates.Empty();
    PCGComponents.Empty();
    TargetWorld.Reset();
}
