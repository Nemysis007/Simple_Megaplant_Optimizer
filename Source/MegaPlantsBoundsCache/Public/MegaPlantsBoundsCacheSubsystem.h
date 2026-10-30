#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MegaPlantsBoundsCacheSubsystem.generated.h"

class UMegaPlantsBoundsCacheHelper;

UCLASS(BlueprintType)
class MEGAPLANTSBOUNDSCACHE_API UMegaPlantsBoundsCacheSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void OnWorldEndPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "Mega Plants|Bounds Cache",
        meta = (WorldContext = "WorldContextObject", DisplayName = "Get Mega Plants Bounds Cache Subsystem"))
    static UMegaPlantsBoundsCacheSubsystem* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Mega Plants|Bounds Cache",
        meta = (Tooltip = "Invalidate plugin-owned overrides after custom instance or wind-bound edits, including same-count instance replacements. Caching rebuilds before a subsequent world tick when enabled."))
    void RefreshCachedBounds();

    UFUNCTION(BlueprintCallable, Category = "Mega Plants|Bounds Cache")
    void LogCacheStatus() const;

private:
    void ShutdownHelper();

    UPROPERTY(Transient)
    TObjectPtr<UMegaPlantsBoundsCacheHelper> Helper;
};
