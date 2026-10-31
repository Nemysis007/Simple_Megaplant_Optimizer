# Simple Megaplant Optimizer — Mega Plants Bounds Cache

[Download v0.1.0 for UE 5.7 Win64](https://github.com/Nemysis007/Simple_Megaplant_Optimizer/releases/tag/v0.1.0). The plugin appears in Unreal Editor as **Mega Plants Bounds Cache**; keep the installed folder name `MegaPlantsBoundsCache`.

A UE 5.7 runtime plugin that caches aggregate bounds for stationary CPU-instanced skinned foliage driven by Dynamic Wind. It avoids repeating the per-instance aggregate bounds calculation while the component's placement is unchanged. Wind continues to animate; component ticks are not disabled.

## Support & Feedback

Having trouble or want to share feedback? [Join the Discord community](https://discord.gg/ehVhy7Fxh) for plugin support and discussion. When reporting a problem, include your Unreal Engine version, plugin version, and the steps that reproduce it.

## Requirements and scope

- Unreal Engine 5.7. Other engine versions have not been validated.
- PCG is a declared plugin dependency for generation lifecycle callbacks.
- Foliage must use `UInstancedSkinnedMeshComponent` with an enabled `UDynamicWindData` provider (or subclass). Dynamic Wind must already be enabled/configured by the project using those assets; this plugin detects it without linking against the DynamicWind module or loading its assets.
- Runtime game/PIE worlds with rendering. Dedicated servers and the non-playing editor world are excluded.
- Normal static meshes, ordinary skeletal characters, GPU-only instances, empty components, compiling assets/providers, and components with existing valid bounds overrides are skipped.

This does not offload wind calculations to a CPU worker, change navigation settings, or modify Unreal Engine source. It targets one repeated bounds-calculation cost, not every source of foliage overhead.

## Install from source

1. Close Unreal Editor and copy this folder to `<YourProject>/Plugins/MegaPlantsBoundsCache`.
2. Install the C++ toolchain required by your UE 5.7 installation. For a C++ project, regenerate project files and build its Editor target. For a Blueprint-only project, either add a C++ class to create a build target or use a precompiled plugin release that matches your engine/platform.
3. Open the project, enable **Mega Plants Bounds Cache** under Edit > Plugins if necessary, and restart when prompted.
4. Open **Project Settings > Plugins > Mega Plants Bounds Cache**.
5. Enable **Cache Foliage Bounds**. Start with **Foliage Bounds Padding = 200 cm**.
6. Play your level. In Output Log, look for `LogMegaPlantsBoundsCache` and `Cached foliage bounds for ... components`.

The cache is off by default. The setting and padding are read before each world tick, so they can be changed during PIE. Console overrides take precedence.

## Install a precompiled release

Extract the release's `MegaPlantsBoundsCache` folder into the project's `Plugins` folder, then follow steps 3–6 above. Use only one copy of the plugin. A binary built for a different engine build/platform may need recompiling. Installation does not add Mega Plants assets, configure PCG graphs, or enable streaming.

## Same-scene before/after comparison

Run these in the PIE game console:

```text
MegaPlants.BoundsCache 0
MegaPlants.BoundsCache.Status
MegaPlants.BoundsCache 1
MegaPlants.BoundsCache.Status
MegaPlants.BoundsCache -1
```

- `0`: remove overrides owned by this plugin and disable caching.
- `1`: enable caching.
- `-1`: follow Project Settings (default). Restore this after testing; console overrides persist across PIE sessions until the editor exits.
- `.Status`: report registered instanced skinned components, instance counts, eligibility, tick state, and plugin-owned versus external/pre-existing bounds overrides.

Allow a few frames after changing the switch before recording; the first aggregate build and clearing the cache still cost time. Compare the same camera, instance count, wind, rendering settings, and streaming state. Record **game-thread milliseconds**, not just FPS. An uncached plugin does not remove another system's pre-existing override.

Observed results in the original test project included approximately 35–36 ms to 2.5–3.5 ms on one scene and 90 ms to 7 ms on a larger scene. These are scene-specific measurements of the original integrated implementation, not guarantees or fresh measurements of this extracted plugin. GPU/render-thread limits still affect overall frame time.

## Padding and exclusions

UE 5.7 Dynamic Wind provides no animation bounds. Cached mesh bounds are expanded by the configured margin on every side to allow for wind displacement. Check strong wind, view edges, shadows, and distant views; increase padding if necessary. Oversized bounds can reduce culling efficiency. A 200 cm default is a starting point, not a guarantee for every asset or wind setup.

Add the actor or component tag `SkipMegaPlantsBoundsCache` to exclude particular foliage. Eligibility changes are checked before world ticks.

## PCG, streaming, and custom edits

The helper discovers components at runtime and checks for new/streamed components once per game second while enabled. It uses weak references so it does not retain streamed-out actors.

PCG start/completion/cancellation/cleanup events invalidate plugin-owned overrides. The helper pauses caching while a discovered PCG component generates or cleans up. Invalidation is world-wide because PCG graphs can target actors other than their owner; unrelated runtime PCG generation may therefore temporarily remove the optimization.

Cheap checks detect changes to component transform, instance count/storage, asset/provider pointer, and eligibility. They do not inspect every instance every frame. This is intended for stationary foliage, not continuously moving instances. Same-count in-place edits, changes to the same asset/provider's internal bounds, and custom runtime generation outside the observed PCG lifecycle require an explicit refresh.

In Blueprints:

1. Use **Get Mega Plants Bounds Cache Subsystem**.
2. Call **Refresh Cached Bounds** after custom instance/animation-bound edits.
3. Use **Log Cache Status** for diagnostics.

The refresh removes owned overrides immediately and permits rebuilding before a subsequent world tick. World end play clears owned overrides while components are still alive; subsystem deinitialization is a fallback. Overrides subsequently supplied by another system are preserved.

## Source layout

```text
MegaPlantsBoundsCache.uplugin
Config/FilterPlugin.ini
Source/MegaPlantsBoundsCache/
  MegaPlantsBoundsCache.Build.cs
  Public/MegaPlantsBoundsCacheSettings.h
  Public/MegaPlantsBoundsCacheSubsystem.h
  Private/MegaPlantsBoundsCacheModule.cpp
  Private/MegaPlantsBoundsCacheSubsystem.cpp
  Private/MegaPlantsBoundsCacheHelper.h
  Private/MegaPlantsBoundsCacheHelper.cpp
```

The helper is extracted from the working minimap implementation. The standalone subsystem replaces the minimap lifecycle wiring; settings, logs, tags, Blueprint access, and console commands are renamed. A local annotation workaround around the experimental UE 5.7 header is retained for MSVC compatibility.

## Package a binary release

From PowerShell, replace the example paths with your own:

```powershell
& 'G:\Engine\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' BuildPlugin `
  '-Plugin=C:\YourRepo\MegaPlantsBoundsCache.uplugin' `
  '-Package=C:\BuildOutput\MegaPlantsBoundsCache' `
  -TargetPlatforms=Win64
```

Use a disposable output directory separate from the source: BuildPlugin clears its output directory. Test the packaged plugin in a clean project before publishing. Keep source in Git; distribute binaries through release downloads. See LICENSE for distribution terms. No third-party foliage assets or engine source are included here.
