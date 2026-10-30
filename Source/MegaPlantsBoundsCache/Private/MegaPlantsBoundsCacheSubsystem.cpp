#include "MegaPlantsBoundsCacheSubsystem.h"
#include "MegaPlantsBoundsCacheHelper.h"
#include "MegaPlantsBoundsCacheSettings.h"
#include "Engine/World.h"

bool UMegaPlantsBoundsCacheSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UMegaPlantsBoundsCacheSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->GetNetMode() != NM_DedicatedServer && Super::ShouldCreateSubsystem(Outer);
}

void UMegaPlantsBoundsCacheSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    ShutdownHelper();
    Helper = NewObject<UMegaPlantsBoundsCacheHelper>(this);
    Helper->Start(&InWorld, UMegaPlantsBoundsCacheSettings::Get()->FoliageBoundsPaddingCm);
}

void UMegaPlantsBoundsCacheSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
    ShutdownHelper();
    Super::OnWorldEndPlay(InWorld);
}

void UMegaPlantsBoundsCacheSubsystem::Deinitialize()
{
    ShutdownHelper();
    Super::Deinitialize();
}

void UMegaPlantsBoundsCacheSubsystem::ShutdownHelper()
{
    if (Helper)
    {
        Helper->Shutdown();
        Helper = nullptr;
    }
}

UMegaPlantsBoundsCacheSubsystem* UMegaPlantsBoundsCacheSubsystem::Get(const UObject* WorldContextObject)
{
    UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    return World ? World->GetSubsystem<UMegaPlantsBoundsCacheSubsystem>() : nullptr;
}

void UMegaPlantsBoundsCacheSubsystem::RefreshCachedBounds()
{
    if (Helper) Helper->Invalidate();
}

void UMegaPlantsBoundsCacheSubsystem::LogCacheStatus() const
{
    if (Helper) Helper->LogStatus();
}
