#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MegaPlantsBoundsCacheSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mega Plants Bounds Cache"))
class MEGAPLANTSBOUNDSCACHE_API UMegaPlantsBoundsCacheSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    static const UMegaPlantsBoundsCacheSettings* Get() { return GetDefault<UMegaPlantsBoundsCacheSettings>(); }
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("Mega Plants Bounds Cache"); }

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Foliage",
        meta = (DisplayName = "Cache Foliage Bounds", Tooltip = "Opt-in optimization for stationary CPU-instanced skinned foliage using Dynamic Wind. Preserves existing overrides and GPU-only instances. Changes apply before the next world tick unless MegaPlants.BoundsCache overrides this setting. PCG regeneration refreshes the cache; call Refresh Cached Bounds after custom instance edits. Add SkipMegaPlantsBoundsCache to an actor or component to exclude it."))
    bool bCacheFoliageBounds = false;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Foliage",
        meta = (DisplayName = "Foliage Bounds Padding", ClampMin = "0.0", Units = "cm", EditCondition = "bCacheFoliageBounds",
            Tooltip = "World-space margin on each side of aggregate mesh bounds. UE 5.7 Dynamic Wind supplies no animation bounds: allow enough space for wind displacement. Larger bounds reduce culling efficiency. Changes refresh the cache before the next world tick."))
    float FoliageBoundsPaddingCm = 200.f;
};
