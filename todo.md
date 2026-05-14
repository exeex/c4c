Status: Active
Source Idea Path: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Verify Phoenix Extraction Scope

# Current Packet

## Just Finished

Step 1: Verify Phoenix Extraction Scope completed. The module directory
inventory contains exactly `module.cpp` and `module.hpp`; `module.hpp` is the
only `.hpp` file and therefore the sole non-helper header. Stage-1 owned
surface is `module.cpp` plus `module.hpp`; no module markdown artifacts are
present in `src/backend/mir/aarch64/module/`.

## Suggested Next

Proceed with the next stage-1 extraction packet using the verified
`module.cpp`/`module.hpp` scope.

## Watchouts

- Treat `module.cpp` as legacy evidence, not the design to patch.
- Prepared BIR should lower directly to MIR machine nodes in the rebuilt
  route.
- Stage 1 extracts evidence only; replacement layout belongs to stage 2.
- No surprise module-surface files were found by the delegated inventory.
- Do not broaden the scope beyond `module.cpp` and `module.hpp` without
  lifecycle repair.

## Proof

Ran the supervisor-selected inventory proof:
`find src/backend/mir/aarch64/module -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.md' \) -printf '%f\n' | sort > test_after.log`.
Proof log: `test_after.log`.
