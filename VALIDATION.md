# Validation and release checklist

## Extraction

- Extracted from the existing UE 5.7 MinimapSystem bounds-cache implementation on 2026-10-30.
- Added standalone module, settings, and world subsystem; no minimap, navigation, vehicle, quest, or UI dependency.
- Original minimap implementation remains in place.
- Standalone Unreal Automation Tool BuildPlugin: SUCCESS (2026-10-30). Compiled Win64 UnrealEditor Development, UnrealGame Development, and UnrealGame Shipping with UE 5.7.4 (changelist 51494982), Visual Studio 2022 toolchain 14.44.35229 and Windows SDK 10.0.22621.0. No minimap module was included in the build host.
- Fresh-project PIE smoke test completed on 2026-10-30 in MegaPlantOP using the precompiled Win64 plugin and matching UE 5.7.4 engine. The user reported successful testing and working console controls.
- Reviewed Output Log evidence from Foliage_Test: four eligible Dynamic Wind components, with instance counts 17,907 / 17,546 / 17,549 / 17,600. Enabling cached all four; disabling changed PreviousCached=4 to CurrentCached=0; re-enabling cached all four again. The log also shows startup using ProjectSetting=On and ConsoleOverride=-1.
- Packaged-game runtime testing and the edge cases below remain unverified. Original integrated-plugin performance measurements are not fresh standalone performance measurements.

## Tests still required in the fresh project

- Verify Project Settings off/on with console override reset to `-1`.
- Check repeated PIE sessions and world travel.
- Check actor/component exclusion tags and existing external overrides.
- Check strong wind, screen edges, shadows, distant views, and padding.
- Check World Partition stream-in/out and PCG regeneration, cancellation, and cleanup.
- Check custom same-count instance edits followed by Blueprint refresh.
- Test a packaged game with the intended distribution engine build.
- Confirm only one bounds-cache implementation is enabled if MinimapSystem is installed too.
- Capture hardware/scene details and fresh measurements before making standalone performance claims.

Compilation alone does not establish runtime performance, visual correctness, platform compatibility beyond the compiled target, or FAB readiness.

