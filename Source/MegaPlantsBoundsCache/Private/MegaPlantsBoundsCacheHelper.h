#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "MegaPlantsBoundsCacheHelper.generated.h"

class UInstancedSkinnedMeshComponent;
class UPCGComponent;
class USkinnedAsset;
class UTransformProviderData;

// Runtime-only state, owned by the standalone bounds-cache world subsystem.
UCLASS()
class UMegaPlantsBoundsCacheHelper : public UObject
{
    GENERATED_BODY()
public:
    virtual UWorld* GetWorld() const override;
    void Start(UWorld* World, float PaddingCm);
    void Shutdown();
    void Invalidate();
    void LogStatus() const;

private:
    struct FCachedBounds
    {
        FBox AppliedBounds = FBox(ForceInit);
        FTransform Transform;
        TWeakObjectPtr<USkinnedAsset> Asset;
        TWeakObjectPtr<UTransformProviderData> Provider;
        const void* InstanceStorage = nullptr;
        int32 InstanceCount = 0;
    };

    void BeforeWorldTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    void DiscoverComponents();
    void OnPCGChanged(UPCGComponent* Component);
    void Restore(UInstancedSkinnedMeshComponent* Component, const FCachedBounds& Cached);
    bool CanCache(UInstancedSkinnedMeshComponent* Component) const;
    void UpdateEnabledState();

    TWeakObjectPtr<UWorld> TargetWorld;
    TMap<TWeakObjectPtr<UInstancedSkinnedMeshComponent>, FCachedBounds> CachedComponents;
    TSet<TWeakObjectPtr<UInstancedSkinnedMeshComponent>> Candidates;
    TSet<TWeakObjectPtr<UPCGComponent>> PCGComponents;
    FDelegateHandle BeforeTickHandle;
    float Padding = 200.f;
    float DiscoveryCountdown = 0.f;
    bool bEnabled = false;
    bool bLoggedInitialState = false;
};
