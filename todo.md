Status: Active
Source Idea Path: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Verify Phoenix Extraction Scope

# Current Packet

## Just Finished

Lifecycle switch only. The active route moved from 224 to phoenix stage 1
extraction for the AArch64 module/MIR lowering rebuild.

## Suggested Next

Delegate Step 1 to verify the module extraction scope and header invariant
before running the phoenix extraction script.

## Watchouts

- Treat `module.cpp` as legacy evidence, not the design to patch.
- Prepared BIR should lower directly to MIR machine nodes in the rebuilt
  route.
- Stage 1 extracts evidence only; replacement layout belongs to stage 2.
- Do not broaden the scope beyond `module.cpp`, `module.hpp`, and module
  markdown artifacts without lifecycle repair.

## Proof

Lifecycle-only switch. No build or test proof required.
